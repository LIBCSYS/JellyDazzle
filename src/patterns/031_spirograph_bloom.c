/* pattern_031 — Spirograph Bloom
 * Port of lab/patterns/031_spirograph_bloom/proto.py
 * Hypotrochoid with breathing gear ratio q and pen offset d, deposited with
 * D6 symmetry (6 rotations x y-mirror = 12 copies) into a 320x240 weight
 * accumulator with exponential age fade, then 1-2-1 blurred, tone-mapped
 * through 1-exp(-a*gain), vignette-blended and bilinearly upscaled.
 * Hue rides the palette ring: a second accumulator carries weight*hue so the
 * per-pixel mean hue survives the accumulation. Accumulator pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <string.h>

#define P31_AW   320
#define P31_AH   240
#define P31_AN   (P31_AW * P31_AH)
#define P31_MAXD 4096
#define P31_SUB  36
#define P31_BASE 260
#define P31_TAU  380.0f
#define P31_GAIN 0.16f
#define P31_SAT  0.95f        /* proto HSV saturation */
#define P31_TSC  585.14f          /* 4096/7 : tone LUT index scale */

static float    p31_aw[P31_AN];   /* weight accumulator      */
static float    p31_ah[P31_AN];   /* weight * hue            */
static float    p31_bw[P31_AN];   /* h-blurred weight        */
static float    p31_bh[P31_AN];   /* h-blurred weight*hue    */
static uint32_t p31_low[P31_AN];
static uint8_t  p31_bg[P31_AN * 3];
static uint8_t  p31_lin[4096];
static uint8_t  p31_gam[256];
static float    p31_sc[6], p31_ss[6];
static int      p31_ready = 0;
static int      p31_lastsl = -1000;
static double   p31_n = 0.0;
static int      p31_uw = -1, p31_uh = -1;
static int32_t  p31_x0[P31_MAXD], p31_y0[P31_MAXD];
static uint8_t  p31_fx[P31_MAXD], p31_fy[P31_MAXD];

static void p31_tabs(void) {
    for (int i = 0; i < 4096; i++)
        p31_lin[i] = (uint8_t)((1.0 - exp(-(double)i * 7.0 / 4096.0)) * 255.0 + 0.5);
    for (int i = 0; i < 256; i++)
        p31_gam[i] = (uint8_t)(pow((double)i / 255.0, 0.85) * 255.0 + 0.5);
    for (int y = 0; y < P31_AH; y++)
        for (int x = 0; x < P31_AW; x++) {
            double dx = (x - P31_AW * 0.5) / P31_AW;
            double dy = (y - P31_AH * 0.5) / P31_AH;
            double e = exp(-(dx * dx + dy * dy) * 3.0) * 255.0;
            int i = (y * P31_AW + x) * 3;
            p31_bg[i + 0] = (uint8_t)(e * 0.04 + 0.5);
            p31_bg[i + 1] = (uint8_t)(e * 0.01 + 0.5);
            p31_bg[i + 2] = (uint8_t)(e * 0.07 + 0.5);
        }
    for (int k = 0; k < 6; k++) {
        double a = k * (3.14159265358979 / 3.0);
        p31_sc[k] = (float)cos(a);
        p31_ss[k] = (float)sin(a);
    }
    p31_ready = 1;
}

static void p31_splat(int xi, int yi, float w, float hw) {
    if ((unsigned)xi >= (unsigned)P31_AW || (unsigned)yi >= (unsigned)P31_AH) return;
    int i = yi * P31_AW + xi;
    p31_aw[i] += w;
    p31_ah[i] += hw;
}

static void p31_emit(double n, float w) {
    double th = n * 0.11;
    double q  = 2.5  + 1.2  * sin(n * 0.0021);
    double d  = 0.35 + 0.25 * sin(n * 0.0009 + 1.7);
    float  x  = (float)(55.2 * cos(th) + 92.0 * d * cos(q * th));
    float  y  = (float)(55.2 * sin(th) - 92.0 * d * sin(q * th));
    float  hw = (float)(th * 0.0028) * w;
    const float ox = P31_AW * 0.5f, oy = P31_AH * 0.5f;
    for (int k = 0; k < 6; k++) {
        float c = p31_sc[k], s = p31_ss[k];
        float ax = x * c - y * s, ay = x * s + y * c;
        float bx = x * c + y * s, by = x * s - y * c;
        p31_splat((int)lrintf(ax + ox), (int)lrintf(ay + oy), w, hw);
        p31_splat((int)lrintf(bx + ox), (int)lrintf(by + oy), w, hw);
    }
}

static void p31_hblur(void) {
    for (int y = 0; y < P31_AH; y++) {
        const float *rw = p31_aw + y * P31_AW, *rh = p31_ah + y * P31_AW;
        float *ow = p31_bw + y * P31_AW, *oh = p31_bh + y * P31_AW;
        ow[0] = 0.75f * rw[0] + 0.25f * rw[1];
        oh[0] = 0.75f * rh[0] + 0.25f * rh[1];
        for (int x = 1; x < P31_AW - 1; x++) {
            ow[x] = 0.5f * rw[x] + 0.25f * (rw[x - 1] + rw[x + 1]);
            oh[x] = 0.5f * rh[x] + 0.25f * (rh[x - 1] + rh[x + 1]);
        }
        ow[P31_AW - 1] = 0.75f * rw[P31_AW - 1] + 0.25f * rw[P31_AW - 2];
        oh[P31_AW - 1] = 0.75f * rh[P31_AW - 1] + 0.25f * rh[P31_AW - 2];
    }
}

static void p31_compose(const uint32_t *pal, float hoff) {
    p31_hblur();
    for (int y = 0; y < P31_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P31_AH - 1) ? y + 1 : y;
        const float *cw = p31_bw + y * P31_AW, *uw = p31_bw + yu * P31_AW,
                    *dw = p31_bw + yd * P31_AW;
        const float *ch = p31_bh + y * P31_AW, *uh = p31_bh + yu * P31_AW,
                    *dh = p31_bh + yd * P31_AW;
        uint32_t *out = p31_low + y * P31_AW;
        const uint8_t *bg = p31_bg + y * P31_AW * 3;
        for (int x = 0; x < P31_AW; x++) {
            float a = 0.5f * cw[x] + 0.25f * (uw[x] + dw[x]);
            float hh = 0.5f * ch[x] + 0.25f * (uh[x] + dh[x]);
            float hue = (a > 1e-6f) ? hh / a : 0.0f;
            int idx = (int)((hue + hoff) * 32768.0f) & JD_PAL_MASK;
            uint32_t c = pal[idx];
            int pr = (c >> 16) & 255, pg = (c >> 8) & 255, pb = c & 255;
            int m = pr > pg ? pr : pg; if (pb > m) m = pb;
            int lo = pr < pg ? pr : pg; if (pb < lo) lo = pb;
            /* Re-cast the palette entry as an HSV colour of value 1 and the
               proto's saturation S:  c' = (1-S) + S*(c-lo)/(m-lo)          */
            float base = a * P31_GAIN * P31_TSC;
            int tr, tg, tb;
            if (m - lo < 1) { tr = tg = tb = (int)base; }
            else {
                float k1 = base * (1.0f - P31_SAT);
                float k2 = base * P31_SAT / (float)(m - lo);
                tr = (int)(k1 + k2 * (pr - lo));
                tg = (int)(k1 + k2 * (pg - lo));
                tb = (int)(k1 + k2 * (pb - lo));
            }
            if (tr > 4095) tr = 4095; if (tg > 4095) tg = 4095; if (tb > 4095) tb = 4095;
            int vr = p31_lin[tr], vg = p31_lin[tg], vb = p31_lin[tb];
            vr += ((255 - vr) * bg[x * 3 + 0]) >> 8;
            vg += ((255 - vg) * bg[x * 3 + 1]) >> 8;
            vb += ((255 - vb) * bg[x * 3 + 2]) >> 8;
            out[x] = 0xFF000000u | ((uint32_t)p31_gam[vr] << 16)
                   | ((uint32_t)p31_gam[vg] << 8) | p31_gam[vb];
        }
    }
}

static uint32_t p31_lerp(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8) & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p31_utab(int w, int h) {
    for (int x = 0; x < w && x < P31_MAXD; x++) {
        long f = ((long)(2 * x + 1) * P31_AW << 15) / w - (1L << 15);
        if (f < 0) f = 0;
        long mx = ((long)(P31_AW - 1)) << 16; if (f > mx) f = mx;
        p31_x0[x] = (int32_t)(f >> 16);
        p31_fx[x] = (uint8_t)((f >> 8) & 255);
    }
    for (int y = 0; y < h && y < P31_MAXD; y++) {
        long f = ((long)(2 * y + 1) * P31_AH << 15) / h - (1L << 15);
        if (f < 0) f = 0;
        long my = ((long)(P31_AH - 1)) << 16; if (f > my) f = my;
        p31_y0[y] = (int32_t)(f >> 16);
        p31_fy[y] = (uint8_t)((f >> 8) & 255);
    }
    p31_uw = w; p31_uh = h;
}

static void p31_upscale(uint32_t *fb, int w, int h) {
    static uint32_t row[P31_AW];
    if (w > P31_MAXD || h > P31_MAXD) {          /* fallback: nearest */
        for (int y = 0; y < h; y++) {
            const uint32_t *s = p31_low + (y * P31_AH / h) * P31_AW;
            uint32_t *d = fb + (long)y * w;
            for (int x = 0; x < w; x++) d[x] = s[x * P31_AW / w];
        }
        return;
    }
    if (w != p31_uw || h != p31_uh) p31_utab(w, h);
    for (int y = 0; y < h; y++) {
        int y0 = p31_y0[y], fy = p31_fy[y];
        int y1 = (y0 < P31_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p31_low + y0 * P31_AW, *r1 = p31_low + y1 * P31_AW;
        if (fy) for (int i = 0; i < P31_AW; i++) row[i] = p31_lerp(r0[i], r1[i], fy);
        else    memcpy(row, r0, sizeof row);
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int x0 = p31_x0[x];
            int x1 = (x0 < P31_AW - 1) ? x0 + 1 : x0;
            d[x] = 0xFF000000u | p31_lerp(row[x0], row[x1], p31_fx[x]);
        }
    }
}

void pattern_031(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p31_ready) p31_tabs();
    if (sl < 2 || sl != p31_lastsl + 1) {
        memset(p31_aw, 0, sizeof p31_aw);
        memset(p31_ah, 0, sizeof p31_ah);
        p31_n = (double)(seed & 2047u);
        for (int f = 0; f < P31_BASE; f++) {
            float wg = expf(-(float)(P31_BASE - f) / P31_TAU);
            for (int i = 0; i < P31_SUB; i++) {
                p31_emit(p31_n + (double)i / P31_SUB, wg);
            }
            p31_n += 1.0;
        }
    } else {
        const float K = 0.99737189f;               /* exp(-1/380) */
        for (int i = 0; i < P31_AN; i++) { p31_aw[i] *= K; p31_ah[i] *= K; }
        for (int i = 0; i < P31_SUB; i++)
            p31_emit(p31_n + (double)i / P31_SUB, 1.0f);
        p31_n += 1.0;
    }
    p31_lastsl = sl;
    p31_compose(pal, (float)(p31_n * 0.0007));
    p31_upscale(fb, w, h);
}
