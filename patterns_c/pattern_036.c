/* pattern_036 — Epicycle Lace
 * Port of lab/patterns/036_epicycle_lace/proto.py
 * Three stacked epicycles (frequencies 1, -4, 9 — pairwise 5 apart, so the
 * raw curve already carries 5-fold symmetry) whose two free phases precess at
 * different rates.  Deposited with D5 symmetry (5 rotations x mirror = 10
 * copies) into a 320x240 weight accumulator with exponential age fade;
 * blurred, tone-mapped through 1-exp(-a*gain), midnight-blue vignette blended
 * and bilinearly upscaled.  A parallel weight*hue accumulator carries the
 * cyan<->violet band LFO.  Accumulator pattern. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define P36_AW   320
#define P36_AH   240
#define P36_AN   (P36_AW * P36_AH)
#define P36_MAXD 4096
#define P36_SUB  34
#define P36_BASE 260
#define P36_TAU  400.0f
#define P36_GAIN 0.10f
#define P36_SAT  0.85f        /* proto HSV saturation */
#define P36_TSC  585.14f          /* 4096/7 : tone LUT index scale */

static float    p36_aw[P36_AN];   /* weight accumulator      */
static float    p36_ah[P36_AN];   /* weight * hue            */
static float    p36_bw[P36_AN];   /* h-blurred weight        */
static float    p36_bh[P36_AN];   /* h-blurred weight*hue    */
static uint32_t p36_low[P36_AN];
static uint8_t  p36_bg[P36_AN * 3];
static uint8_t  p36_lin[4096];
static uint8_t  p36_gam[256];
static float    p36_sc[5], p36_ss[5];
static int      p36_ready = 0;
static int      p36_lastsl = -1000;
static double   p36_n = 0.0;
static int      p36_uw = -1, p36_uh = -1;
static int32_t  p36_x0[P36_MAXD], p36_y0[P36_MAXD];
static uint8_t  p36_fx[P36_MAXD], p36_fy[P36_MAXD];

static void p36_tabs(void) {
    for (int i = 0; i < 4096; i++)
        p36_lin[i] = (uint8_t)((1.0 - exp(-(double)i * 7.0 / 4096.0)) * 255.0 + 0.5);
    for (int i = 0; i < 256; i++)
        p36_gam[i] = (uint8_t)(pow((double)i / 255.0, 0.85) * 255.0 + 0.5);
    for (int y = 0; y < P36_AH; y++)
        for (int x = 0; x < P36_AW; x++) {
            double dx = (x - P36_AW * 0.5) / P36_AW;
            double dy = (y - P36_AH * 0.5) / P36_AH;
            double e = exp(-(dx * dx + dy * dy) * 3.0) * 255.0;
            int i = (y * P36_AW + x) * 3;
            p36_bg[i + 0] = 0;
            p36_bg[i + 1] = (uint8_t)(e * 0.02 + 0.5);
            p36_bg[i + 2] = (uint8_t)(e * 0.07 + 0.5);
        }
    for (int k = 0; k < 5; k++) {
        double a = k * (6.28318530717959 / 5.0);
        p36_sc[k] = (float)cos(a);
        p36_ss[k] = (float)sin(a);
    }
    p36_ready = 1;
}

static void p36_splat(int xi, int yi, float w, float hw) {
    if ((unsigned)xi >= (unsigned)P36_AW || (unsigned)yi >= (unsigned)P36_AH) return;
    int i = yi * P36_AW + xi;
    p36_aw[i] += w;
    p36_ah[i] += hw;
}

static void p36_emit(double n, float w) {
    double th = n * 0.09;
    double p1 = n * 0.0011;              /* the two free phases precess    */
    double p2 = n * 0.0007 + 2.0;        /* at different rates             */
    double a1 = -4.0 * th + p1, a2 = 9.0 * th + p2;
    float  x  = (float)(56.0 * cos(th) + 34.0 * cos(a1) + 18.0 * cos(a2));
    float  y  = (float)(56.0 * sin(th) + 34.0 * sin(a1) + 18.0 * sin(a2));
    float  hw = (float)(0.45 + 0.28 * sin(th * 0.07 + n * 0.0005)) * w;
    const float ox = P36_AW * 0.5f, oy = P36_AH * 0.5f;
    for (int k = 0; k < 5; k++) {
        float c = p36_sc[k], s = p36_ss[k];
        float ax = x * c - y * s, ay = x * s + y * c;
        float bx = x * c + y * s, by = x * s - y * c;
        p36_splat((int)lrintf(ax + ox), (int)lrintf(ay + oy), w, hw);
        p36_splat((int)lrintf(bx + ox), (int)lrintf(by + oy), w, hw);
    }
}

static void p36_hblur(void) {
    for (int y = 0; y < P36_AH; y++) {
        const float *rw = p36_aw + y * P36_AW, *rh = p36_ah + y * P36_AW;
        float *ow = p36_bw + y * P36_AW, *oh = p36_bh + y * P36_AW;
        ow[0] = 0.75f * rw[0] + 0.25f * rw[1];
        oh[0] = 0.75f * rh[0] + 0.25f * rh[1];
        for (int x = 1; x < P36_AW - 1; x++) {
            ow[x] = 0.5f * rw[x] + 0.25f * (rw[x - 1] + rw[x + 1]);
            oh[x] = 0.5f * rh[x] + 0.25f * (rh[x - 1] + rh[x + 1]);
        }
        ow[P36_AW - 1] = 0.75f * rw[P36_AW - 1] + 0.25f * rw[P36_AW - 2];
        oh[P36_AW - 1] = 0.75f * rh[P36_AW - 1] + 0.25f * rh[P36_AW - 2];
    }
}

static void p36_compose(const uint32_t *pal, float hoff) {
    p36_hblur();
    for (int y = 0; y < P36_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P36_AH - 1) ? y + 1 : y;
        const float *cw = p36_bw + y * P36_AW, *uw = p36_bw + yu * P36_AW,
                    *dw = p36_bw + yd * P36_AW;
        const float *ch = p36_bh + y * P36_AW, *uh = p36_bh + yu * P36_AW,
                    *dh = p36_bh + yd * P36_AW;
        uint32_t *out = p36_low + y * P36_AW;
        const uint8_t *bg = p36_bg + y * P36_AW * 3;
        for (int x = 0; x < P36_AW; x++) {
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
            float base = a * P36_GAIN * P36_TSC;
            int tr, tg, tb;
            if (m - lo < 1) { tr = tg = tb = (int)base; }
            else {
                float k1 = base * (1.0f - P36_SAT);
                float k2 = base * P36_SAT / (float)(m - lo);
                tr = (int)(k1 + k2 * (pr - lo));
                tg = (int)(k1 + k2 * (pg - lo));
                tb = (int)(k1 + k2 * (pb - lo));
            }
            if (tr > 4095) tr = 4095; if (tg > 4095) tg = 4095; if (tb > 4095) tb = 4095;
            int vr = p36_lin[tr], vg = p36_lin[tg], vb = p36_lin[tb];
            vr += ((255 - vr) * bg[x * 3 + 0]) >> 8;
            vg += ((255 - vg) * bg[x * 3 + 1]) >> 8;
            vb += ((255 - vb) * bg[x * 3 + 2]) >> 8;
            out[x] = 0xFF000000u | ((uint32_t)p36_gam[vr] << 16)
                   | ((uint32_t)p36_gam[vg] << 8) | p36_gam[vb];
        }
    }
}

static uint32_t p36_lerp(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8) & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p36_utab(int w, int h) {
    for (int x = 0; x < w && x < P36_MAXD; x++) {
        long f = ((long)(2 * x + 1) * P36_AW << 15) / w - (1L << 15);
        if (f < 0) f = 0;
        long mx = ((long)(P36_AW - 1)) << 16; if (f > mx) f = mx;
        p36_x0[x] = (int32_t)(f >> 16);
        p36_fx[x] = (uint8_t)((f >> 8) & 255);
    }
    for (int y = 0; y < h && y < P36_MAXD; y++) {
        long f = ((long)(2 * y + 1) * P36_AH << 15) / h - (1L << 15);
        if (f < 0) f = 0;
        long my = ((long)(P36_AH - 1)) << 16; if (f > my) f = my;
        p36_y0[y] = (int32_t)(f >> 16);
        p36_fy[y] = (uint8_t)((f >> 8) & 255);
    }
    p36_uw = w; p36_uh = h;
}

static void p36_upscale(uint32_t *fb, int w, int h) {
    static uint32_t row[P36_AW];
    if (w > P36_MAXD || h > P36_MAXD) {          /* fallback: nearest */
        for (int y = 0; y < h; y++) {
            const uint32_t *s = p36_low + (y * P36_AH / h) * P36_AW;
            uint32_t *d = fb + (long)y * w;
            for (int x = 0; x < w; x++) d[x] = s[x * P36_AW / w];
        }
        return;
    }
    if (w != p36_uw || h != p36_uh) p36_utab(w, h);
    for (int y = 0; y < h; y++) {
        int y0 = p36_y0[y], fy = p36_fy[y];
        int y1 = (y0 < P36_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p36_low + y0 * P36_AW, *r1 = p36_low + y1 * P36_AW;
        if (fy) for (int i = 0; i < P36_AW; i++) row[i] = p36_lerp(r0[i], r1[i], fy);
        else    memcpy(row, r0, sizeof row);
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int x0 = p36_x0[x];
            int x1 = (x0 < P36_AW - 1) ? x0 + 1 : x0;
            d[x] = 0xFF000000u | p36_lerp(row[x0], row[x1], p36_fx[x]);
        }
    }
}

void pattern_036(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p36_ready) p36_tabs();
    if (sl < 2 || sl != p36_lastsl + 1) {
        memset(p36_aw, 0, sizeof p36_aw);
        memset(p36_ah, 0, sizeof p36_ah);
        p36_n = (double)(seed & 2047u);
        for (int f = 0; f < P36_BASE; f++) {
            float wg = expf(-(float)(P36_BASE - f) / P36_TAU);
            for (int i = 0; i < P36_SUB; i++) {
                p36_emit(p36_n + (double)i / P36_SUB, wg);
            }
            p36_n += 1.0;
        }
    } else {
        const float K = 0.99750312f;               /* exp(-1/400) */
        for (int i = 0; i < P36_AN; i++) { p36_aw[i] *= K; p36_ah[i] *= K; }
        for (int i = 0; i < P36_SUB; i++)
            p36_emit(p36_n + (double)i / P36_SUB, 1.0f);
        p36_n += 1.0;
    }
    p36_lastsl = sl;
    p36_compose(pal, 0.0f);
    p36_upscale(fb, w, h);
}
