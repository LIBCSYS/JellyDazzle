/* pattern_033 — Rose Engine
 * Port of lab/patterns/033_rose_engine/proto.py
 * Offset rhodonea r = A*cos(k*theta) + off with a fractional, slowly drifting
 * petal count k (the rose never closes), deposited with D7 symmetry (7
 * rotations x mirror = 14 copies) into a 320x240 weight accumulator with
 * exponential age fade; blurred, tone-mapped through 1-exp(-a*gain), warm
 * vignette-blended and bilinearly upscaled.  Hue is radius-locked (crimson
 * core -> gold rim) and carried by a parallel weight*hue accumulator.
 * Accumulator pattern. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define P33_AW   320
#define P33_AH   240
#define P33_AN   (P33_AW * P33_AH)
#define P33_MAXD 4096
#define P33_SUB  32
#define P33_BASE 260
#define P33_TAU  360.0f
#define P33_GAIN 0.07f
#define P33_HSPAN 0.15f       /* palette arc from core to rim */
#define P33_SAT  0.88f        /* proto HSV saturation */
#define P33_TSC  585.14f          /* 4096/7 : tone LUT index scale */

static float    p33_aw[P33_AN];   /* weight accumulator      */
static float    p33_ah[P33_AN];   /* weight * hue            */
static float    p33_bw[P33_AN];   /* h-blurred weight        */
static float    p33_bh[P33_AN];   /* h-blurred weight*hue    */
static uint32_t p33_low[P33_AN];
static uint8_t  p33_bg[P33_AN * 3];
static uint8_t  p33_lin[4096];
static uint8_t  p33_gam[256];
static float    p33_sc[7], p33_ss[7];
static int      p33_ready = 0;
static int      p33_lastsl = -1000;
static double   p33_n = 0.0;
static int      p33_uw = -1, p33_uh = -1;
static int32_t  p33_x0[P33_MAXD], p33_y0[P33_MAXD];
static uint8_t  p33_fx[P33_MAXD], p33_fy[P33_MAXD];

static void p33_tabs(void) {
    for (int i = 0; i < 4096; i++)
        p33_lin[i] = (uint8_t)((1.0 - exp(-(double)i * 7.0 / 4096.0)) * 255.0 + 0.5);
    for (int i = 0; i < 256; i++)
        p33_gam[i] = (uint8_t)(pow((double)i / 255.0, 0.85) * 255.0 + 0.5);
    for (int y = 0; y < P33_AH; y++)
        for (int x = 0; x < P33_AW; x++) {
            double dx = (x - P33_AW * 0.5) / P33_AW;
            double dy = (y - P33_AH * 0.5) / P33_AH;
            double e = exp(-(dx * dx + dy * dy) * 3.0) * 255.0;
            int i = (y * P33_AW + x) * 3;
            p33_bg[i + 0] = (uint8_t)(e * 0.07 + 0.5);
            p33_bg[i + 1] = (uint8_t)(e * 0.02 + 0.5);
            p33_bg[i + 2] = (uint8_t)(e * 0.01 + 0.5);
        }
    for (int k = 0; k < 7; k++) {
        double a = k * (6.28318530717959 / 7.0);
        p33_sc[k] = (float)cos(a);
        p33_ss[k] = (float)sin(a);
    }
    p33_ready = 1;
}

static void p33_splat(int xi, int yi, float w, float hw) {
    if ((unsigned)xi >= (unsigned)P33_AW || (unsigned)yi >= (unsigned)P33_AH) return;
    int i = yi * P33_AW + xi;
    p33_aw[i] += w;
    p33_ah[i] += hw;
}

static void p33_emit(double n, float w) {
    double th = n * 0.19;
    double k  = 2.5 + 1.5 * sin(n * 0.0006);           /* fractional petals   */
    double rr = (76.0 + 8.0 * sin(n * 0.0013)) * cos(k * th)
              + 18.0 * sin(n * 0.0004);
    float  x  = (float)(rr * cos(th));
    float  y  = (float)(rr * sin(th));
    float  t  = (float)((rr + 94.0) * (1.0 / 188.0));  /* radius -> hue ramp  */
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    /* radius-locked colour: a narrow palette arc (core -> rim) so the
       medallion always reads as one scheme, plus the slow global drift
       applied at compose time. */
    float  hw = (P33_HSPAN * t) * w;
    const float ox = P33_AW * 0.5f, oy = P33_AH * 0.5f;
    for (int j = 0; j < 7; j++) {
        float c = p33_sc[j], s = p33_ss[j];
        float ax = x * c - y * s, ay = x * s + y * c;
        float bx = x * c + y * s, by = x * s - y * c;
        p33_splat((int)lrintf(ax + ox), (int)lrintf(ay + oy), w, hw);
        p33_splat((int)lrintf(bx + ox), (int)lrintf(by + oy), w, hw);
    }
}

static void p33_hblur(void) {
    for (int y = 0; y < P33_AH; y++) {
        const float *rw = p33_aw + y * P33_AW, *rh = p33_ah + y * P33_AW;
        float *ow = p33_bw + y * P33_AW, *oh = p33_bh + y * P33_AW;
        ow[0] = 0.75f * rw[0] + 0.25f * rw[1];
        oh[0] = 0.75f * rh[0] + 0.25f * rh[1];
        for (int x = 1; x < P33_AW - 1; x++) {
            ow[x] = 0.5f * rw[x] + 0.25f * (rw[x - 1] + rw[x + 1]);
            oh[x] = 0.5f * rh[x] + 0.25f * (rh[x - 1] + rh[x + 1]);
        }
        ow[P33_AW - 1] = 0.75f * rw[P33_AW - 1] + 0.25f * rw[P33_AW - 2];
        oh[P33_AW - 1] = 0.75f * rh[P33_AW - 1] + 0.25f * rh[P33_AW - 2];
    }
}

static void p33_compose(const uint32_t *pal, float hoff) {
    p33_hblur();
    for (int y = 0; y < P33_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P33_AH - 1) ? y + 1 : y;
        const float *cw = p33_bw + y * P33_AW, *uw = p33_bw + yu * P33_AW,
                    *dw = p33_bw + yd * P33_AW;
        const float *ch = p33_bh + y * P33_AW, *uh = p33_bh + yu * P33_AW,
                    *dh = p33_bh + yd * P33_AW;
        uint32_t *out = p33_low + y * P33_AW;
        const uint8_t *bg = p33_bg + y * P33_AW * 3;
        for (int x = 0; x < P33_AW; x++) {
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
            float base = a * P33_GAIN * P33_TSC;
            int tr, tg, tb;
            if (m - lo < 1) { tr = tg = tb = (int)base; }
            else {
                float k1 = base * (1.0f - P33_SAT);
                float k2 = base * P33_SAT / (float)(m - lo);
                tr = (int)(k1 + k2 * (pr - lo));
                tg = (int)(k1 + k2 * (pg - lo));
                tb = (int)(k1 + k2 * (pb - lo));
            }
            if (tr > 4095) tr = 4095; if (tg > 4095) tg = 4095; if (tb > 4095) tb = 4095;
            int vr = p33_lin[tr], vg = p33_lin[tg], vb = p33_lin[tb];
            vr += ((255 - vr) * bg[x * 3 + 0]) >> 8;
            vg += ((255 - vg) * bg[x * 3 + 1]) >> 8;
            vb += ((255 - vb) * bg[x * 3 + 2]) >> 8;
            out[x] = 0xFF000000u | ((uint32_t)p33_gam[vr] << 16)
                   | ((uint32_t)p33_gam[vg] << 8) | p33_gam[vb];
        }
    }
}

static uint32_t p33_lerp(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8) & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p33_utab(int w, int h) {
    for (int x = 0; x < w && x < P33_MAXD; x++) {
        long f = ((long)(2 * x + 1) * P33_AW << 15) / w - (1L << 15);
        if (f < 0) f = 0;
        long mx = ((long)(P33_AW - 1)) << 16; if (f > mx) f = mx;
        p33_x0[x] = (int32_t)(f >> 16);
        p33_fx[x] = (uint8_t)((f >> 8) & 255);
    }
    for (int y = 0; y < h && y < P33_MAXD; y++) {
        long f = ((long)(2 * y + 1) * P33_AH << 15) / h - (1L << 15);
        if (f < 0) f = 0;
        long my = ((long)(P33_AH - 1)) << 16; if (f > my) f = my;
        p33_y0[y] = (int32_t)(f >> 16);
        p33_fy[y] = (uint8_t)((f >> 8) & 255);
    }
    p33_uw = w; p33_uh = h;
}

static void p33_upscale(uint32_t *fb, int w, int h) {
    static uint32_t row[P33_AW];
    if (w > P33_MAXD || h > P33_MAXD) {          /* fallback: nearest */
        for (int y = 0; y < h; y++) {
            const uint32_t *s = p33_low + (y * P33_AH / h) * P33_AW;
            uint32_t *d = fb + (long)y * w;
            for (int x = 0; x < w; x++) d[x] = s[x * P33_AW / w];
        }
        return;
    }
    if (w != p33_uw || h != p33_uh) p33_utab(w, h);
    for (int y = 0; y < h; y++) {
        int y0 = p33_y0[y], fy = p33_fy[y];
        int y1 = (y0 < P33_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p33_low + y0 * P33_AW, *r1 = p33_low + y1 * P33_AW;
        if (fy) for (int i = 0; i < P33_AW; i++) row[i] = p33_lerp(r0[i], r1[i], fy);
        else    memcpy(row, r0, sizeof row);
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int x0 = p33_x0[x];
            int x1 = (x0 < P33_AW - 1) ? x0 + 1 : x0;
            d[x] = 0xFF000000u | p33_lerp(row[x0], row[x1], p33_fx[x]);
        }
    }
}

void pattern_033(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p33_ready) p33_tabs();
    if (sl < 2 || sl != p33_lastsl + 1) {
        memset(p33_aw, 0, sizeof p33_aw);
        memset(p33_ah, 0, sizeof p33_ah);
        p33_n = (double)(seed & 2047u);
        for (int f = 0; f < P33_BASE; f++) {
            float wg = expf(-(float)(P33_BASE - f) / P33_TAU);
            for (int i = 0; i < P33_SUB; i++) {
                p33_emit(p33_n + (double)i / P33_SUB, wg);
            }
            p33_n += 1.0;
        }
    } else {
        const float K = 0.99722606f;               /* exp(-1/360) */
        for (int i = 0; i < P33_AN; i++) { p33_aw[i] *= K; p33_ah[i] *= K; }
        for (int i = 0; i < P33_SUB; i++)
            p33_emit(p33_n + (double)i / P33_SUB, 1.0f);
        p33_n += 1.0;
    }
    p33_lastsl = sl;
    p33_compose(pal, (float)(p33_n * 0.0006));
    p33_upscale(fb, w, h);
}
