/* pattern_037 — Pendulum Web
 * Port of lab/patterns/037_pendulum_web/proto.py
 * String art strung between two counter-precessing Lissajous curves: every
 * tick three rungs are stretched from a point on curve A to the matching
 * point on curve B and rasterised by fixed-step DDA into a 320x240 RGB
 * accumulator with exponential age fade (per rung, tau 800), copied at 180
 * degrees, blurred, tone-mapped through 1-exp(-a*gain), vignette-blended and
 * bilinearly upscaled.  Each rung carries a two-tone gradient along its
 * length so crossings blend to secondary tones.  Accumulator pattern. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define P37_AW    320
#define P37_AH    240
#define P37_AN    (P37_AW * P37_AH)
#define P37_MAXD  4096
#define P37_BASE  280
#define P37_RPF   3            /* rungs per frame                  */
#define P37_STEPS 40           /* DDA samples per rung             */
#define P37_GAIN  0.26f
#define P37_SAT   0.90f
#define P37_TSC   585.14f      /* 4096/7 : tone LUT index scale    */

static float    p37_ar[P37_AN], p37_ag[P37_AN], p37_ab[P37_AN];
static float    p37_br[P37_AN], p37_bg_[P37_AN], p37_bb[P37_AN];
static uint32_t p37_low[P37_AN];
static uint8_t  p37_bgv[P37_AN * 3];
static uint8_t  p37_lin[4096];
static uint8_t  p37_gam[256];
static int      p37_ready = 0;
static int      p37_lastsl = -1000;
static double   p37_m = 0.0;      /* rung counter */
static double   p37_t = 0.0;      /* frame counter within segment */
static int      p37_uw = -1, p37_uh = -1;
static int32_t  p37_x0[P37_MAXD], p37_y0[P37_MAXD];
static uint8_t  p37_fx[P37_MAXD], p37_fy[P37_MAXD];

static void p37_tabs(void) {
    for (int i = 0; i < 4096; i++)
        p37_lin[i] = (uint8_t)((1.0 - exp(-(double)i * 7.0 / 4096.0)) * 255.0 + 0.5);
    for (int i = 0; i < 256; i++)
        p37_gam[i] = (uint8_t)(pow((double)i / 255.0, 0.85) * 255.0 + 0.5);
    for (int y = 0; y < P37_AH; y++)
        for (int x = 0; x < P37_AW; x++) {
            double dx = (x - P37_AW * 0.5) / P37_AW;
            double dy = (y - P37_AH * 0.5) / P37_AH;
            double e = exp(-(dx * dx + dy * dy) * 3.0) * 255.0;
            int i = (y * P37_AW + x) * 3;
            p37_bgv[i + 0] = (uint8_t)(e * 0.03 + 0.5);
            p37_bgv[i + 1] = 0;
            p37_bgv[i + 2] = (uint8_t)(e * 0.05 + 0.5);
        }
    p37_ready = 1;
}

/* one rung, deposited twice (identity + 180-degree rotation) */
static void p37_rung(double m, float w, double hbase, const uint32_t *pal) {
    double sp = m * 0.213;
    double ph = m * 0.0009;              /* curves precess in opposition */
    float x1 = (float)(105.0 * sin(2.0 * sp + ph));
    float y1 = (float)( 80.0 * sin(3.0 * sp));
    float x2 = (float)(105.0 * sin(3.0 * sp + 1.2 - ph));
    float y2 = (float)( 80.0 * sin(5.0 * sp + 0.5));

    const float ox = P37_AW * 0.5f, oy = P37_AH * 0.5f;
    const float du = 1.0f / (float)(P37_STEPS - 1);
    float dx = (x2 - x1) * du, dy = (y2 - y1) * du;
    float px = x1, py = y1;
    /* palette index walks a fixed step along the rung: cyan end -> magenta */
    int   idx  = (int)(hbase * 32768.0);
    int   didx = (int)(0.35 * 32768.0 * du);
    for (int i = 0; i < P37_STEPS; i++, px += dx, py += dy, idx += didx) {
        uint32_t c = pal[idx & JD_PAL_MASK];
        int pr = (c >> 16) & 255, pg = (c >> 8) & 255, pb = c & 255;
        int hi = pr > pg ? pr : pg; if (pb > hi) hi = pb;
        int lo = pr < pg ? pr : pg; if (pb < lo) lo = pb;
        float cr, cg, cb;
        if (hi - lo < 1) { cr = cg = cb = w; }
        else {
            float k1 = (1.0f - P37_SAT) * w, k2 = P37_SAT * w / (float)(hi - lo);
            cr = k1 + k2 * (pr - lo);
            cg = k1 + k2 * (pg - lo);
            cb = k1 + k2 * (pb - lo);
        }
        int xa = (int)lrintf(ox + px), xb = (int)lrintf(ox - px);
        int ya = (int)lrintf(oy + py), yb = (int)lrintf(oy - py);
        if ((unsigned)xa < (unsigned)P37_AW && (unsigned)ya < (unsigned)P37_AH) {
            int j = ya * P37_AW + xa;
            p37_ar[j] += cr; p37_ag[j] += cg; p37_ab[j] += cb;
        }
        if ((unsigned)xb < (unsigned)P37_AW && (unsigned)yb < (unsigned)P37_AH) {
            int j = yb * P37_AW + xb;
            p37_ar[j] += cr; p37_ag[j] += cg; p37_ab[j] += cb;
        }
    }
}

static void p37_hblur1(const float *src, float *dst) {
    for (int y = 0; y < P37_AH; y++) {
        const float *r = src + y * P37_AW;
        float *o = dst + y * P37_AW;
        o[0] = 0.75f * r[0] + 0.25f * r[1];
        for (int x = 1; x < P37_AW - 1; x++)
            o[x] = 0.5f * r[x] + 0.25f * (r[x - 1] + r[x + 1]);
        o[P37_AW - 1] = 0.75f * r[P37_AW - 1] + 0.25f * r[P37_AW - 2];
    }
}

static void p37_compose(void) {
    p37_hblur1(p37_ar, p37_br);
    p37_hblur1(p37_ag, p37_bg_);
    p37_hblur1(p37_ab, p37_bb);
    for (int y = 0; y < P37_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P37_AH - 1) ? y + 1 : y;
        const float *r0 = p37_br + y * P37_AW, *r1 = p37_br + yu * P37_AW,
                    *r2 = p37_br + yd * P37_AW;
        const float *g0 = p37_bg_ + y * P37_AW, *g1 = p37_bg_ + yu * P37_AW,
                    *g2 = p37_bg_ + yd * P37_AW;
        const float *b0 = p37_bb + y * P37_AW, *b1 = p37_bb + yu * P37_AW,
                    *b2 = p37_bb + yd * P37_AW;
        uint32_t *out = p37_low + y * P37_AW;
        const uint8_t *bg = p37_bgv + y * P37_AW * 3;
        for (int x = 0; x < P37_AW; x++) {
            float ar = 0.5f * r0[x] + 0.25f * (r1[x] + r2[x]);
            float ag = 0.5f * g0[x] + 0.25f * (g1[x] + g2[x]);
            float ab = 0.5f * b0[x] + 0.25f * (b1[x] + b2[x]);
            int tr = (int)(ar * (P37_GAIN * P37_TSC));
            int tg = (int)(ag * (P37_GAIN * P37_TSC));
            int tb = (int)(ab * (P37_GAIN * P37_TSC));
            if (tr > 4095) tr = 4095; if (tg > 4095) tg = 4095; if (tb > 4095) tb = 4095;
            int vr = p37_lin[tr], vg = p37_lin[tg], vb = p37_lin[tb];
            vr += ((255 - vr) * bg[x * 3 + 0]) >> 8;
            vg += ((255 - vg) * bg[x * 3 + 1]) >> 8;
            vb += ((255 - vb) * bg[x * 3 + 2]) >> 8;
            out[x] = 0xFF000000u | ((uint32_t)p37_gam[vr] << 16)
                   | ((uint32_t)p37_gam[vg] << 8) | p37_gam[vb];
        }
    }
}

static uint32_t p37_lerp(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8) & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p37_utab(int w, int h) {
    for (int x = 0; x < w && x < P37_MAXD; x++) {
        long f = ((long)(2 * x + 1) * P37_AW << 15) / w - (1L << 15);
        if (f < 0) f = 0;
        long mx = ((long)(P37_AW - 1)) << 16; if (f > mx) f = mx;
        p37_x0[x] = (int32_t)(f >> 16);
        p37_fx[x] = (uint8_t)((f >> 8) & 255);
    }
    for (int y = 0; y < h && y < P37_MAXD; y++) {
        long f = ((long)(2 * y + 1) * P37_AH << 15) / h - (1L << 15);
        if (f < 0) f = 0;
        long my = ((long)(P37_AH - 1)) << 16; if (f > my) f = my;
        p37_y0[y] = (int32_t)(f >> 16);
        p37_fy[y] = (uint8_t)((f >> 8) & 255);
    }
    p37_uw = w; p37_uh = h;
}

static void p37_upscale(uint32_t *fb, int w, int h) {
    static uint32_t row[P37_AW];
    if (w > P37_MAXD || h > P37_MAXD) {
        for (int y = 0; y < h; y++) {
            const uint32_t *s = p37_low + (y * P37_AH / h) * P37_AW;
            uint32_t *d = fb + (long)y * w;
            for (int x = 0; x < w; x++) d[x] = s[x * P37_AW / w];
        }
        return;
    }
    if (w != p37_uw || h != p37_uh) p37_utab(w, h);
    for (int y = 0; y < h; y++) {
        int y0 = p37_y0[y], fy = p37_fy[y];
        int y1 = (y0 < P37_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p37_low + y0 * P37_AW, *r1 = p37_low + y1 * P37_AW;
        if (fy) for (int i = 0; i < P37_AW; i++) row[i] = p37_lerp(r0[i], r1[i], fy);
        else    memcpy(row, r0, sizeof row);
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int x0 = p37_x0[x];
            int x1 = (x0 < P37_AW - 1) ? x0 + 1 : x0;
            d[x] = 0xFF000000u | p37_lerp(row[x0], row[x1], p37_fx[x]);
        }
    }
}

void pattern_037(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p37_ready) p37_tabs();
    if (sl < 2 || sl != p37_lastsl + 1) {
        memset(p37_ar, 0, sizeof p37_ar);
        memset(p37_ag, 0, sizeof p37_ag);
        memset(p37_ab, 0, sizeof p37_ab);
        p37_m = (double)(seed & 8191u);
        p37_t = 0.0;
        for (int f = 0; f < P37_BASE; f++) {
            float wg = expf(-(float)((P37_BASE - f) * P37_RPF) / 800.0f);
            for (int c = 0; c < P37_RPF; c++, p37_m += 1.0)
                p37_rung(p37_m, wg, 0.55 + p37_t * 0.0004, pal);
            p37_t += 1.0;
        }
    } else {
        const float K = 0.99625702f;               /* exp(-3/800) per frame */
        for (int i = 0; i < P37_AN; i++) {
            p37_ar[i] *= K; p37_ag[i] *= K; p37_ab[i] *= K;
        }
        for (int c = 0; c < P37_RPF; c++, p37_m += 1.0)
            p37_rung(p37_m, 1.0f, 0.55 + p37_t * 0.0004, pal);
        p37_t += 1.0;
    }
    p37_lastsl = sl;
    p37_compose();
    p37_upscale(fb, w, h);
}
