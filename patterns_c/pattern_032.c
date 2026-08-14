/* pattern_032 — Harmonograph Veil
 * Port of lab/patterns/032_harmonograph_veil/proto.py
 * Undamped four-oscillator harmonograph (two per axis, the second detuned
 * 2.0025:1 so the veil slowly precesses), deposited 4-fold mirrored into a
 * 320x240 weight accumulator with exponential age fade; 1-2-1 blurred,
 * tone-mapped through 1-exp(-a*gain), vignette-blended, bilinearly upscaled.
 * A parallel weight*hue accumulator carries the teal->violet hue band.
 * Accumulator pattern. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define P32_AW   320
#define P32_AH   240
#define P32_AN   (P32_AW * P32_AH)
#define P32_MAXD 4096
#define P32_SUB  40
#define P32_BASE 280
#define P32_TAU  420.0f
#define P32_GAIN 0.22f
#define P32_SAT  0.80f        /* proto HSV saturation */
#define P32_TSC  585.14f          /* 4096/7 : tone LUT index scale */

static float    p32_aw[P32_AN];   /* weight accumulator      */
static float    p32_ah[P32_AN];   /* weight * hue            */
static float    p32_bw[P32_AN];   /* h-blurred weight        */
static float    p32_bh[P32_AN];   /* h-blurred weight*hue    */
static uint32_t p32_low[P32_AN];
static uint8_t  p32_bg[P32_AN * 3];
static uint8_t  p32_lin[4096];
static uint8_t  p32_gam[256];
static int      p32_ready = 0;
static int      p32_lastsl = -1000;
static double   p32_n = 0.0;
static double   p32_n0 = 0.0;   /* segment origin: hue LFO is relative */
static int      p32_uw = -1, p32_uh = -1;
static int32_t  p32_x0[P32_MAXD], p32_y0[P32_MAXD];
static uint8_t  p32_fx[P32_MAXD], p32_fy[P32_MAXD];

static void p32_tabs(void) {
    for (int i = 0; i < 4096; i++)
        p32_lin[i] = (uint8_t)((1.0 - exp(-(double)i * 7.0 / 4096.0)) * 255.0 + 0.5);
    for (int i = 0; i < 256; i++)
        p32_gam[i] = (uint8_t)(pow((double)i / 255.0, 0.85) * 255.0 + 0.5);
    for (int y = 0; y < P32_AH; y++)
        for (int x = 0; x < P32_AW; x++) {
            double dx = (x - P32_AW * 0.5) / P32_AW;
            double dy = (y - P32_AH * 0.5) / P32_AH;
            double e = exp(-(dx * dx + dy * dy) * 3.0) * 255.0;
            int i = (y * P32_AW + x) * 3;
            p32_bg[i + 0] = (uint8_t)(e * 0.01 + 0.5);
            p32_bg[i + 1] = (uint8_t)(e * 0.04 + 0.5);
            p32_bg[i + 2] = (uint8_t)(e * 0.06 + 0.5);
        }
    p32_ready = 1;
}

static void p32_splat(int xi, int yi, float w, float hw) {
    if ((unsigned)xi >= (unsigned)P32_AW || (unsigned)yi >= (unsigned)P32_AH) return;
    int i = yi * P32_AW + xi;
    p32_aw[i] += w;
    p32_ah[i] += hw;
}

static void p32_emit(double n, float w) {
    double sp = n * 0.23;
    double p2 = n * 0.0011;
    float  x  = (float)(74.0 * sin(sp + 0.3) + 46.0 * sin(2.0025 * sp + p2));
    float  y  = (float)(62.0 * sin(1.503 * sp)
                      + 40.0 * sin(2.996 * sp + 1.1 + 0.5 * p2));
    double hn = n - p32_n0;   /* hue band starts at teal each segment */
    float  hu = (float)(0.52 + 0.18 * sin(hn * 0.0007) + 0.04 * sin(sp * 0.11));
    float  hw = hu * w;
    const float ox = P32_AW * 0.5f, oy = P32_AH * 0.5f;
    int xa = (int)lrintf(ox + x), xb = (int)lrintf(ox - x);
    int ya = (int)lrintf(oy + y), yb = (int)lrintf(oy - y);
    p32_splat(xa, ya, w, hw);
    p32_splat(xb, ya, w, hw);
    p32_splat(xa, yb, w, hw);
    p32_splat(xb, yb, w, hw);
}

static void p32_hblur(void) {
    for (int y = 0; y < P32_AH; y++) {
        const float *rw = p32_aw + y * P32_AW, *rh = p32_ah + y * P32_AW;
        float *ow = p32_bw + y * P32_AW, *oh = p32_bh + y * P32_AW;
        ow[0] = 0.75f * rw[0] + 0.25f * rw[1];
        oh[0] = 0.75f * rh[0] + 0.25f * rh[1];
        for (int x = 1; x < P32_AW - 1; x++) {
            ow[x] = 0.5f * rw[x] + 0.25f * (rw[x - 1] + rw[x + 1]);
            oh[x] = 0.5f * rh[x] + 0.25f * (rh[x - 1] + rh[x + 1]);
        }
        ow[P32_AW - 1] = 0.75f * rw[P32_AW - 1] + 0.25f * rw[P32_AW - 2];
        oh[P32_AW - 1] = 0.75f * rh[P32_AW - 1] + 0.25f * rh[P32_AW - 2];
    }
}

static void p32_compose(const uint32_t *pal, float hoff) {
    p32_hblur();
    for (int y = 0; y < P32_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P32_AH - 1) ? y + 1 : y;
        const float *cw = p32_bw + y * P32_AW, *uw = p32_bw + yu * P32_AW,
                    *dw = p32_bw + yd * P32_AW;
        const float *ch = p32_bh + y * P32_AW, *uh = p32_bh + yu * P32_AW,
                    *dh = p32_bh + yd * P32_AW;
        uint32_t *out = p32_low + y * P32_AW;
        const uint8_t *bg = p32_bg + y * P32_AW * 3;
        for (int x = 0; x < P32_AW; x++) {
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
            float base = a * P32_GAIN * P32_TSC;
            int tr, tg, tb;
            if (m - lo < 1) { tr = tg = tb = (int)base; }
            else {
                float k1 = base * (1.0f - P32_SAT);
                float k2 = base * P32_SAT / (float)(m - lo);
                tr = (int)(k1 + k2 * (pr - lo));
                tg = (int)(k1 + k2 * (pg - lo));
                tb = (int)(k1 + k2 * (pb - lo));
            }
            if (tr > 4095) tr = 4095; if (tg > 4095) tg = 4095; if (tb > 4095) tb = 4095;
            int vr = p32_lin[tr], vg = p32_lin[tg], vb = p32_lin[tb];
            vr += ((255 - vr) * bg[x * 3 + 0]) >> 8;
            vg += ((255 - vg) * bg[x * 3 + 1]) >> 8;
            vb += ((255 - vb) * bg[x * 3 + 2]) >> 8;
            out[x] = 0xFF000000u | ((uint32_t)p32_gam[vr] << 16)
                   | ((uint32_t)p32_gam[vg] << 8) | p32_gam[vb];
        }
    }
}

static uint32_t p32_lerp(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8) & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p32_utab(int w, int h) {
    for (int x = 0; x < w && x < P32_MAXD; x++) {
        long f = ((long)(2 * x + 1) * P32_AW << 15) / w - (1L << 15);
        if (f < 0) f = 0;
        long mx = ((long)(P32_AW - 1)) << 16; if (f > mx) f = mx;
        p32_x0[x] = (int32_t)(f >> 16);
        p32_fx[x] = (uint8_t)((f >> 8) & 255);
    }
    for (int y = 0; y < h && y < P32_MAXD; y++) {
        long f = ((long)(2 * y + 1) * P32_AH << 15) / h - (1L << 15);
        if (f < 0) f = 0;
        long my = ((long)(P32_AH - 1)) << 16; if (f > my) f = my;
        p32_y0[y] = (int32_t)(f >> 16);
        p32_fy[y] = (uint8_t)((f >> 8) & 255);
    }
    p32_uw = w; p32_uh = h;
}

static void p32_upscale(uint32_t *fb, int w, int h) {
    static uint32_t row[P32_AW];
    if (w > P32_MAXD || h > P32_MAXD) {          /* fallback: nearest */
        for (int y = 0; y < h; y++) {
            const uint32_t *s = p32_low + (y * P32_AH / h) * P32_AW;
            uint32_t *d = fb + (long)y * w;
            for (int x = 0; x < w; x++) d[x] = s[x * P32_AW / w];
        }
        return;
    }
    if (w != p32_uw || h != p32_uh) p32_utab(w, h);
    for (int y = 0; y < h; y++) {
        int y0 = p32_y0[y], fy = p32_fy[y];
        int y1 = (y0 < P32_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p32_low + y0 * P32_AW, *r1 = p32_low + y1 * P32_AW;
        if (fy) for (int i = 0; i < P32_AW; i++) row[i] = p32_lerp(r0[i], r1[i], fy);
        else    memcpy(row, r0, sizeof row);
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int x0 = p32_x0[x];
            int x1 = (x0 < P32_AW - 1) ? x0 + 1 : x0;
            d[x] = 0xFF000000u | p32_lerp(row[x0], row[x1], p32_fx[x]);
        }
    }
}

void pattern_032(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p32_ready) p32_tabs();
    if (sl < 2 || sl != p32_lastsl + 1) {
        memset(p32_aw, 0, sizeof p32_aw);
        memset(p32_ah, 0, sizeof p32_ah);
        p32_n = (double)(seed & 2047u);
        p32_n0 = p32_n;
        for (int f = 0; f < P32_BASE; f++) {
            float wg = expf(-(float)(P32_BASE - f) / P32_TAU);
            for (int i = 0; i < P32_SUB; i++) {
                p32_emit(p32_n + (double)i / P32_SUB, wg);
            }
            p32_n += 1.0;
        }
    } else {
        const float K = 0.99762183f;               /* exp(-1/420) */
        for (int i = 0; i < P32_AN; i++) { p32_aw[i] *= K; p32_ah[i] *= K; }
        for (int i = 0; i < P32_SUB; i++)
            p32_emit(p32_n + (double)i / P32_SUB, 1.0f);
        p32_n += 1.0;
    }
    p32_lastsl = sl;
    p32_compose(pal, 0.0f);
    p32_upscale(fb, w, h);
}
