/* pattern_040 — Maurer Rose
 * Port of lab/patterns/040_maurer_rose/proto.py
 * A Maurer walk over a sine rose: segment m joins the rose points at
 * theta = m*D and (m+1)*D, with the step angle D and the petal count k both
 * crawling, so the mandala dissolves through star-polygon phases and
 * re-crystallizes.  Segments are rasterised by fixed-step DDA into a 320x240
 * RGB accumulator with exponential age fade (per segment, tau 950), mirrored
 * in y, blurred, tone-mapped through 1-exp(-a*gain), teal-vignette blended
 * and bilinearly upscaled.  Hue is radius-locked, giving concentric colour
 * bands.  Accumulator pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <string.h>

#define P40_AW    320
#define P40_AH    240
#define P40_AN    (P40_AW * P40_AH)
#define P40_MAXD  4096
#define P40_BASE  270
#define P40_SPF   4            /* rose segments per frame          */
#define P40_STEPS 34           /* DDA samples per segment          */
#define P40_A     102.0        /* rose amplitude */
#define P40_GAIN  0.24f
#define P40_SAT   0.92f
#define P40_TSC   585.14f      /* 4096/7 : tone LUT index scale    */

static float    p40_ar[P40_AN], p40_ag[P40_AN], p40_ab[P40_AN];
static float    p40_br[P40_AN], p40_bg_[P40_AN], p40_bb[P40_AN];
static uint32_t p40_low[P40_AN];
static uint8_t  p40_bgv[P40_AN * 3];
static uint8_t  p40_lin[4096];
static uint8_t  p40_gam[256];
static int      p40_ready = 0;
static int      p40_lastsl = -1000;
static double   p40_m = 0.0;      /* segment counter */
static double   p40_t = 0.0;      /* frame counter within segment */
static int      p40_uw = -1, p40_uh = -1;
static int32_t  p40_x0[P40_MAXD], p40_y0[P40_MAXD];
static uint8_t  p40_fx[P40_MAXD], p40_fy[P40_MAXD];

static void p40_tabs(void) {
    for (int i = 0; i < 4096; i++)
        p40_lin[i] = (uint8_t)((1.0 - exp(-(double)i * 7.0 / 4096.0)) * 255.0 + 0.5);
    for (int i = 0; i < 256; i++)
        p40_gam[i] = (uint8_t)(pow((double)i / 255.0, 0.85) * 255.0 + 0.5);
    for (int y = 0; y < P40_AH; y++)
        for (int x = 0; x < P40_AW; x++) {
            double dx = (x - P40_AW * 0.5) / P40_AW;
            double dy = (y - P40_AH * 0.5) / P40_AH;
            double e = exp(-(dx * dx + dy * dy) * 3.0) * 255.0;
            int i = (y * P40_AW + x) * 3;
            p40_bgv[i + 0] = 0;
            p40_bgv[i + 1] = (uint8_t)(e * 0.04 + 0.5);
            p40_bgv[i + 2] = (uint8_t)(e * 0.04 + 0.5);
        }
    p40_ready = 1;
}

/* one rose chord, deposited twice (y-mirror) */
static void p40_seg(double m, float w, double hdrift, const uint32_t *pal) {
    double D  = 1.2399 + 0.12 * sin(m * 0.0005);     /* step angle drifts  */
    double kk = 4.0    + 2.0  * sin(m * 0.00023);    /* petal count drifts */
    double t1 = m * D, t2 = (m + 1.0) * D;
    double r1 = P40_A * sin(kk * t1), r2 = P40_A * sin(kk * t2);
    float x1 = (float)(r1 * cos(t1)), y1 = (float)(r1 * sin(t1));
    float x2 = (float)(r2 * cos(t2)), y2 = (float)(r2 * sin(t2));

    double ar1 = r1 < 0 ? -r1 : r1, ar2 = r2 < 0 ? -r2 : r2;
    double rr  = 0.5 * (ar1 + ar2) / P40_A;          /* radius -> hue band */
    double hue = 0.52 + 0.38 * rr + hdrift;
    uint32_t c = pal[(int)(hue * 32768.0) & JD_PAL_MASK];
    int pr = (c >> 16) & 255, pg = (c >> 8) & 255, pb = c & 255;
    int hi = pr > pg ? pr : pg; if (pb > hi) hi = pb;
    int lo = pr < pg ? pr : pg; if (pb < lo) lo = pb;
    float cr, cg, cb;
    if (hi - lo < 1) { cr = cg = cb = w; }
    else {
        float k1 = (1.0f - P40_SAT) * w, k2 = P40_SAT * w / (float)(hi - lo);
        cr = k1 + k2 * (pr - lo);
        cg = k1 + k2 * (pg - lo);
        cb = k1 + k2 * (pb - lo);
    }

    const float ox = P40_AW * 0.5f, oy = P40_AH * 0.5f;
    const float du = 1.0f / (float)(P40_STEPS - 1);
    float dx = (x2 - x1) * du, dy = (y2 - y1) * du;
    float px = x1, py = y1;
    for (int i = 0; i < P40_STEPS; i++, px += dx, py += dy) {
        int xi = (int)lrintf(ox + px);
        int ya = (int)lrintf(oy + py), yb = (int)lrintf(oy - py);
        if ((unsigned)xi < (unsigned)P40_AW) {
            if ((unsigned)ya < (unsigned)P40_AH) {
                int j = ya * P40_AW + xi;
                p40_ar[j] += cr; p40_ag[j] += cg; p40_ab[j] += cb;
            }
            if ((unsigned)yb < (unsigned)P40_AH) {
                int j = yb * P40_AW + xi;
                p40_ar[j] += cr; p40_ag[j] += cg; p40_ab[j] += cb;
            }
        }
    }
}

static void p40_hblur1(const float *src, float *dst) {
    for (int y = 0; y < P40_AH; y++) {
        const float *r = src + y * P40_AW;
        float *o = dst + y * P40_AW;
        o[0] = 0.75f * r[0] + 0.25f * r[1];
        for (int x = 1; x < P40_AW - 1; x++)
            o[x] = 0.5f * r[x] + 0.25f * (r[x - 1] + r[x + 1]);
        o[P40_AW - 1] = 0.75f * r[P40_AW - 1] + 0.25f * r[P40_AW - 2];
    }
}

static void p40_compose(void) {
    p40_hblur1(p40_ar, p40_br);
    p40_hblur1(p40_ag, p40_bg_);
    p40_hblur1(p40_ab, p40_bb);
    for (int y = 0; y < P40_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P40_AH - 1) ? y + 1 : y;
        const float *r0 = p40_br + y * P40_AW, *r1 = p40_br + yu * P40_AW,
                    *r2 = p40_br + yd * P40_AW;
        const float *g0 = p40_bg_ + y * P40_AW, *g1 = p40_bg_ + yu * P40_AW,
                    *g2 = p40_bg_ + yd * P40_AW;
        const float *b0 = p40_bb + y * P40_AW, *b1 = p40_bb + yu * P40_AW,
                    *b2 = p40_bb + yd * P40_AW;
        uint32_t *out = p40_low + y * P40_AW;
        const uint8_t *bg = p40_bgv + y * P40_AW * 3;
        for (int x = 0; x < P40_AW; x++) {
            float ar = 0.5f * r0[x] + 0.25f * (r1[x] + r2[x]);
            float ag = 0.5f * g0[x] + 0.25f * (g1[x] + g2[x]);
            float ab = 0.5f * b0[x] + 0.25f * (b1[x] + b2[x]);
            int tr = (int)(ar * (P40_GAIN * P40_TSC));
            int tg = (int)(ag * (P40_GAIN * P40_TSC));
            int tb = (int)(ab * (P40_GAIN * P40_TSC));
            if (tr > 4095) tr = 4095; if (tg > 4095) tg = 4095; if (tb > 4095) tb = 4095;
            int vr = p40_lin[tr], vg = p40_lin[tg], vb = p40_lin[tb];
            vr += ((255 - vr) * bg[x * 3 + 0]) >> 8;
            vg += ((255 - vg) * bg[x * 3 + 1]) >> 8;
            vb += ((255 - vb) * bg[x * 3 + 2]) >> 8;
            out[x] = 0xFF000000u | ((uint32_t)p40_gam[vr] << 16)
                   | ((uint32_t)p40_gam[vg] << 8) | p40_gam[vb];
        }
    }
}

static uint32_t p40_lerp(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8) & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p40_utab(int w, int h) {
    for (int x = 0; x < w && x < P40_MAXD; x++) {
        long f = ((long)(2 * x + 1) * P40_AW << 15) / w - (1L << 15);
        if (f < 0) f = 0;
        long mx = ((long)(P40_AW - 1)) << 16; if (f > mx) f = mx;
        p40_x0[x] = (int32_t)(f >> 16);
        p40_fx[x] = (uint8_t)((f >> 8) & 255);
    }
    for (int y = 0; y < h && y < P40_MAXD; y++) {
        long f = ((long)(2 * y + 1) * P40_AH << 15) / h - (1L << 15);
        if (f < 0) f = 0;
        long my = ((long)(P40_AH - 1)) << 16; if (f > my) f = my;
        p40_y0[y] = (int32_t)(f >> 16);
        p40_fy[y] = (uint8_t)((f >> 8) & 255);
    }
    p40_uw = w; p40_uh = h;
}

static void p40_upscale(uint32_t *fb, int w, int h) {
    static uint32_t row[P40_AW];
    if (w > P40_MAXD || h > P40_MAXD) {
        for (int y = 0; y < h; y++) {
            const uint32_t *s = p40_low + (y * P40_AH / h) * P40_AW;
            uint32_t *d = fb + (long)y * w;
            for (int x = 0; x < w; x++) d[x] = s[x * P40_AW / w];
        }
        return;
    }
    if (w != p40_uw || h != p40_uh) p40_utab(w, h);
    for (int y = 0; y < h; y++) {
        int y0 = p40_y0[y], fy = p40_fy[y];
        int y1 = (y0 < P40_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p40_low + y0 * P40_AW, *r1 = p40_low + y1 * P40_AW;
        if (fy) for (int i = 0; i < P40_AW; i++) row[i] = p40_lerp(r0[i], r1[i], fy);
        else    memcpy(row, r0, sizeof row);
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int x0 = p40_x0[x];
            int x1 = (x0 < P40_AW - 1) ? x0 + 1 : x0;
            d[x] = 0xFF000000u | p40_lerp(row[x0], row[x1], p40_fx[x]);
        }
    }
}

void pattern_040(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p40_ready) p40_tabs();
    if (sl < 2 || sl != p40_lastsl + 1) {
        memset(p40_ar, 0, sizeof p40_ar);
        memset(p40_ag, 0, sizeof p40_ag);
        memset(p40_ab, 0, sizeof p40_ab);
        p40_m = (double)(seed & 8191u);
        p40_t = 0.0;
        for (int f = 0; f < P40_BASE; f++) {
            float wg = expf(-(float)((P40_BASE - f) * P40_SPF) / 950.0f);
            for (int c = 0; c < P40_SPF; c++, p40_m += 1.0)
                p40_seg(p40_m, wg, p40_t * 0.0005, pal);
            p40_t += 1.0;
        }
    } else {
        const float K = 0.99579884f;               /* exp(-4/950) per frame */
        for (int i = 0; i < P40_AN; i++) {
            p40_ar[i] *= K; p40_ag[i] *= K; p40_ab[i] *= K;
        }
        for (int c = 0; c < P40_SPF; c++, p40_m += 1.0)
            p40_seg(p40_m, 1.0f, p40_t * 0.0005, pal);
        p40_t += 1.0;
    }
    p40_lastsl = sl;
    p40_compose();
    p40_upscale(fb, w, h);
}
