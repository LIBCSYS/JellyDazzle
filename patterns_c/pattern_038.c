/* pattern_038 — Guilloche Rings
 * Port of lab/patterns/038_guilloche_rings/proto.py
 * Five interleaved engine-turned bands: substep n belongs to ring n mod 5,
 * each ring a rhodonea ribbon r = R0 + 9*sin(11*theta + crawl) + 5*sin(17*theta
 * - crawl) with its own breathing base radius, fixed hue and saturation.
 * Deposited (plus an x-mirror) into a 320x240 RGB accumulator with
 * exponential age fade, blurred, tone-mapped through 1-exp(-a*gain),
 * bronze-vignette blended and bilinearly upscaled.  Accumulator pattern. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define P38_AW    320
#define P38_AH    240
#define P38_AN    (P38_AW * P38_AH)
#define P38_MAXD  4096
#define P38_BASE  260
#define P38_SUB   40           /* substeps per frame               */
#define P38_GAIN  0.20f
#define P38_TSC   585.14f      /* 4096/7 : tone LUT index scale    */

static float    p38_ar[P38_AN], p38_ag[P38_AN], p38_ab[P38_AN];
static float    p38_br[P38_AN], p38_bg_[P38_AN], p38_bb[P38_AN];
static uint32_t p38_low[P38_AN];
static uint8_t  p38_bgv[P38_AN * 3];
static uint8_t  p38_lin[4096];
static uint8_t  p38_gam[256];
static const double p38_r0[5]  = { 34.0, 52.0, 68.0, 84.0, 100.0 };
static const double p38_hue[5] = { 0.11, 0.05, 0.48, 0.14, 0.58 };
static const float  p38_sat[5] = { 0.85f, 0.90f, 0.80f, 0.35f, 0.80f };
static int      p38_ready = 0;
static int      p38_lastsl = -1000;
static double   p38_n = 0.0;      /* substep-time counter (frames) */
static int      p38_uw = -1, p38_uh = -1;
static int32_t  p38_x0[P38_MAXD], p38_y0[P38_MAXD];
static uint8_t  p38_fx[P38_MAXD], p38_fy[P38_MAXD];

static void p38_tabs(void) {
    for (int i = 0; i < 4096; i++)
        p38_lin[i] = (uint8_t)((1.0 - exp(-(double)i * 7.0 / 4096.0)) * 255.0 + 0.5);
    for (int i = 0; i < 256; i++)
        p38_gam[i] = (uint8_t)(pow((double)i / 255.0, 0.85) * 255.0 + 0.5);
    for (int y = 0; y < P38_AH; y++)
        for (int x = 0; x < P38_AW; x++) {
            double dx = (x - P38_AW * 0.5) / P38_AW;
            double dy = (y - P38_AH * 0.5) / P38_AH;
            double e = exp(-(dx * dx + dy * dy) * 3.0) * 255.0;
            int i = (y * P38_AW + x) * 3;
            p38_bgv[i + 0] = (uint8_t)(e * 0.04 + 0.5);
            p38_bgv[i + 1] = (uint8_t)(e * 0.03 + 0.5);
            p38_bgv[i + 2] = (uint8_t)(e * 0.01 + 0.5);
        }
    p38_ready = 1;
}

/* one guilloche point on ring `ring`, deposited twice (x-mirror) */
static void p38_emit(double n, int ring, float w, const uint32_t *pal) {
    double th = n * 0.37;
    double R0 = p38_r0[ring] + 4.0 * sin(n * 0.0008 + ring);
    double r  = R0 + 9.0 * sin(11.0 * th + n * 0.0014 * (ring + 1))
                   + 5.0 * sin(17.0 * th - n * 0.0009);
    float  x  = (float)(r * cos(th));
    float  y  = (float)(r * sin(th));

    double hue = p38_hue[ring] + 0.02 * sin(n * 0.0005 + ring * 2.0);
    uint32_t c = pal[(int)(hue * 32768.0) & JD_PAL_MASK];
    int pr = (c >> 16) & 255, pg = (c >> 8) & 255, pb = c & 255;
    int hi = pr > pg ? pr : pg; if (pb > hi) hi = pb;
    int lo = pr < pg ? pr : pg; if (pb < lo) lo = pb;
    float S = p38_sat[ring];
    float cr, cg, cb;
    if (hi - lo < 1) { cr = cg = cb = w; }
    else {                       /* value 1, the ring's saturation, hue kept */
        float k1 = (1.0f - S) * w, k2 = S * w / (float)(hi - lo);
        cr = k1 + k2 * (pr - lo);
        cg = k1 + k2 * (pg - lo);
        cb = k1 + k2 * (pb - lo);
    }
    const float ox = P38_AW * 0.5f, oy = P38_AH * 0.5f;
    int xa = (int)lrintf(ox + x), xb = (int)lrintf(ox - x);
    int yy = (int)lrintf(oy + y);
    if ((unsigned)yy < (unsigned)P38_AH) {
        if ((unsigned)xa < (unsigned)P38_AW) {
            int j = yy * P38_AW + xa;
            p38_ar[j] += cr; p38_ag[j] += cg; p38_ab[j] += cb;
        }
        if ((unsigned)xb < (unsigned)P38_AW) {
            int j = yy * P38_AW + xb;
            p38_ar[j] += cr; p38_ag[j] += cg; p38_ab[j] += cb;
        }
    }
}

static void p38_hblur1(const float *src, float *dst) {
    for (int y = 0; y < P38_AH; y++) {
        const float *r = src + y * P38_AW;
        float *o = dst + y * P38_AW;
        o[0] = 0.75f * r[0] + 0.25f * r[1];
        for (int x = 1; x < P38_AW - 1; x++)
            o[x] = 0.5f * r[x] + 0.25f * (r[x - 1] + r[x + 1]);
        o[P38_AW - 1] = 0.75f * r[P38_AW - 1] + 0.25f * r[P38_AW - 2];
    }
}

static void p38_compose(void) {
    p38_hblur1(p38_ar, p38_br);
    p38_hblur1(p38_ag, p38_bg_);
    p38_hblur1(p38_ab, p38_bb);
    for (int y = 0; y < P38_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P38_AH - 1) ? y + 1 : y;
        const float *r0 = p38_br + y * P38_AW, *r1 = p38_br + yu * P38_AW,
                    *r2 = p38_br + yd * P38_AW;
        const float *g0 = p38_bg_ + y * P38_AW, *g1 = p38_bg_ + yu * P38_AW,
                    *g2 = p38_bg_ + yd * P38_AW;
        const float *b0 = p38_bb + y * P38_AW, *b1 = p38_bb + yu * P38_AW,
                    *b2 = p38_bb + yd * P38_AW;
        uint32_t *out = p38_low + y * P38_AW;
        const uint8_t *bg = p38_bgv + y * P38_AW * 3;
        for (int x = 0; x < P38_AW; x++) {
            float ar = 0.5f * r0[x] + 0.25f * (r1[x] + r2[x]);
            float ag = 0.5f * g0[x] + 0.25f * (g1[x] + g2[x]);
            float ab = 0.5f * b0[x] + 0.25f * (b1[x] + b2[x]);
            int tr = (int)(ar * (P38_GAIN * P38_TSC));
            int tg = (int)(ag * (P38_GAIN * P38_TSC));
            int tb = (int)(ab * (P38_GAIN * P38_TSC));
            if (tr > 4095) tr = 4095; if (tg > 4095) tg = 4095; if (tb > 4095) tb = 4095;
            int vr = p38_lin[tr], vg = p38_lin[tg], vb = p38_lin[tb];
            vr += ((255 - vr) * bg[x * 3 + 0]) >> 8;
            vg += ((255 - vg) * bg[x * 3 + 1]) >> 8;
            vb += ((255 - vb) * bg[x * 3 + 2]) >> 8;
            out[x] = 0xFF000000u | ((uint32_t)p38_gam[vr] << 16)
                   | ((uint32_t)p38_gam[vg] << 8) | p38_gam[vb];
        }
    }
}

static uint32_t p38_lerp(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8) & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p38_utab(int w, int h) {
    for (int x = 0; x < w && x < P38_MAXD; x++) {
        long f = ((long)(2 * x + 1) * P38_AW << 15) / w - (1L << 15);
        if (f < 0) f = 0;
        long mx = ((long)(P38_AW - 1)) << 16; if (f > mx) f = mx;
        p38_x0[x] = (int32_t)(f >> 16);
        p38_fx[x] = (uint8_t)((f >> 8) & 255);
    }
    for (int y = 0; y < h && y < P38_MAXD; y++) {
        long f = ((long)(2 * y + 1) * P38_AH << 15) / h - (1L << 15);
        if (f < 0) f = 0;
        long my = ((long)(P38_AH - 1)) << 16; if (f > my) f = my;
        p38_y0[y] = (int32_t)(f >> 16);
        p38_fy[y] = (uint8_t)((f >> 8) & 255);
    }
    p38_uw = w; p38_uh = h;
}

static void p38_upscale(uint32_t *fb, int w, int h) {
    static uint32_t row[P38_AW];
    if (w > P38_MAXD || h > P38_MAXD) {
        for (int y = 0; y < h; y++) {
            const uint32_t *s = p38_low + (y * P38_AH / h) * P38_AW;
            uint32_t *d = fb + (long)y * w;
            for (int x = 0; x < w; x++) d[x] = s[x * P38_AW / w];
        }
        return;
    }
    if (w != p38_uw || h != p38_uh) p38_utab(w, h);
    for (int y = 0; y < h; y++) {
        int y0 = p38_y0[y], fy = p38_fy[y];
        int y1 = (y0 < P38_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p38_low + y0 * P38_AW, *r1 = p38_low + y1 * P38_AW;
        if (fy) for (int i = 0; i < P38_AW; i++) row[i] = p38_lerp(r0[i], r1[i], fy);
        else    memcpy(row, r0, sizeof row);
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int x0 = p38_x0[x];
            int x1 = (x0 < P38_AW - 1) ? x0 + 1 : x0;
            d[x] = 0xFF000000u | p38_lerp(row[x0], row[x1], p38_fx[x]);
        }
    }
}

void pattern_038(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p38_ready) p38_tabs();
    if (sl < 2 || sl != p38_lastsl + 1) {
        memset(p38_ar, 0, sizeof p38_ar);
        memset(p38_ag, 0, sizeof p38_ag);
        memset(p38_ab, 0, sizeof p38_ab);
        p38_n = (double)(seed & 2047u);
        for (int f = 0; f < P38_BASE; f++) {
            float wg = expf(-(float)(P38_BASE - f) / 380.0f);
            for (int i = 0; i < P38_SUB; i++)
                p38_emit(p38_n + (double)i / P38_SUB, i % 5, wg, pal);
            p38_n += 1.0;
        }
    } else {
        const float K = 0.99737189f;               /* exp(-1/380) */
        for (int i = 0; i < P38_AN; i++) {
            p38_ar[i] *= K; p38_ag[i] *= K; p38_ab[i] *= K;
        }
        for (int i = 0; i < P38_SUB; i++)
            p38_emit(p38_n + (double)i / P38_SUB, i % 5, 1.0f, pal);
        p38_n += 1.0;
    }
    p38_lastsl = sl;
    p38_compose();
    p38_upscale(fb, w, h);
}
