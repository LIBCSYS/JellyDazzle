/* pattern_039 — Butterfly Lace
 * Port of lab/patterns/039_butterfly_lace/proto.py
 * Temple Fay's butterfly curve, r = 26*(e^sin(th) - 2cos(4th) + sin^5((2th-pi)/24)),
 * with its internal phases sliding so nested translucent butterflies bloom
 * inside one another.  Deposited with an x-mirror into a 320x240 weight
 * accumulator with exponential age fade; blurred, tone-mapped through
 * 1-exp(-a*gain), plum-vignette blended and bilinearly upscaled.  A parallel
 * weight*hue accumulator carries the indigo<->magenta jewel arc.
 * Accumulator pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <string.h>

#define P39_AW   320
#define P39_AH   240
#define P39_AN   (P39_AW * P39_AH)
#define P39_MAXD 4096
#define P39_SUB  38
#define P39_BASE 280
#define P39_TAU  420.0f
#define P39_GAIN 0.20f
#define P39_SAT  0.80f        /* proto HSV saturation */
#define P39_TSC  585.14f          /* 4096/7 : tone LUT index scale */

static float    p39_aw[P39_AN];   /* weight accumulator      */
static float    p39_ah[P39_AN];   /* weight * hue            */
static float    p39_bw[P39_AN];   /* h-blurred weight        */
static float    p39_bh[P39_AN];   /* h-blurred weight*hue    */
static uint32_t p39_low[P39_AN];
static uint8_t  p39_bg[P39_AN * 3];
static uint8_t  p39_lin[4096];
static uint8_t  p39_gam[256];
static int      p39_ready = 0;
static int      p39_lastsl = -1000;
static double   p39_n = 0.0;
static int      p39_uw = -1, p39_uh = -1;
static int32_t  p39_x0[P39_MAXD], p39_y0[P39_MAXD];
static uint8_t  p39_fx[P39_MAXD], p39_fy[P39_MAXD];

static void p39_tabs(void) {
    for (int i = 0; i < 4096; i++)
        p39_lin[i] = (uint8_t)((1.0 - exp(-(double)i * 7.0 / 4096.0)) * 255.0 + 0.5);
    for (int i = 0; i < 256; i++)
        p39_gam[i] = (uint8_t)(pow((double)i / 255.0, 0.85) * 255.0 + 0.5);
    for (int y = 0; y < P39_AH; y++)
        for (int x = 0; x < P39_AW; x++) {
            double dx = (x - P39_AW * 0.5) / P39_AW;
            double dy = (y - P39_AH * 0.5) / P39_AH;
            double e = exp(-(dx * dx + dy * dy) * 3.0) * 255.0;
            int i = (y * P39_AW + x) * 3;
            p39_bg[i + 0] = (uint8_t)(e * 0.04 + 0.5);
            p39_bg[i + 1] = 0;
            p39_bg[i + 2] = (uint8_t)(e * 0.06 + 0.5);
        }
    p39_ready = 1;
}

static void p39_splat(int xi, int yi, float w, float hw) {
    if ((unsigned)xi >= (unsigned)P39_AW || (unsigned)yi >= (unsigned)P39_AH) return;
    int i = yi * P39_AW + xi;
    p39_aw[i] += w;
    p39_ah[i] += hw;
}

static void p39_emit(double n, float w) {
    double th = n * 0.16;
    double ph = n * 0.0006;                       /* wing phases slide */
    double t5 = sin((2.0 * th - 3.14159265358979) / 24.0);
    double t5b = t5 * t5; t5b = t5b * t5b * t5;   /* sin^5 */
    double r  = 26.0 * (exp(sin(th + 3.0 * ph))
                      - 2.0 * cos(4.0 * th + 5.0 * ph) + t5b);
    float  x  = (float)(r * sin(th));
    float  y  = (float)(-r * cos(th) + 18.0);
    float  hw = (float)(0.75 + 0.18 * sin(th * 0.5 + 2.0 * ph)) * w;
    const float ox = P39_AW * 0.5f, oy = P39_AH * 0.5f;
    int yy = (int)lrintf(oy + y);
    p39_splat((int)lrintf(ox + x), yy, w, hw);
    p39_splat((int)lrintf(ox - x), yy, w, hw);
}

static void p39_hblur(void) {
    for (int y = 0; y < P39_AH; y++) {
        const float *rw = p39_aw + y * P39_AW, *rh = p39_ah + y * P39_AW;
        float *ow = p39_bw + y * P39_AW, *oh = p39_bh + y * P39_AW;
        ow[0] = 0.75f * rw[0] + 0.25f * rw[1];
        oh[0] = 0.75f * rh[0] + 0.25f * rh[1];
        for (int x = 1; x < P39_AW - 1; x++) {
            ow[x] = 0.5f * rw[x] + 0.25f * (rw[x - 1] + rw[x + 1]);
            oh[x] = 0.5f * rh[x] + 0.25f * (rh[x - 1] + rh[x + 1]);
        }
        ow[P39_AW - 1] = 0.75f * rw[P39_AW - 1] + 0.25f * rw[P39_AW - 2];
        oh[P39_AW - 1] = 0.75f * rh[P39_AW - 1] + 0.25f * rh[P39_AW - 2];
    }
}

static void p39_compose(const uint32_t *pal, float hoff) {
    p39_hblur();
    for (int y = 0; y < P39_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P39_AH - 1) ? y + 1 : y;
        const float *cw = p39_bw + y * P39_AW, *uw = p39_bw + yu * P39_AW,
                    *dw = p39_bw + yd * P39_AW;
        const float *ch = p39_bh + y * P39_AW, *uh = p39_bh + yu * P39_AW,
                    *dh = p39_bh + yd * P39_AW;
        uint32_t *out = p39_low + y * P39_AW;
        const uint8_t *bg = p39_bg + y * P39_AW * 3;
        for (int x = 0; x < P39_AW; x++) {
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
            float base = a * P39_GAIN * P39_TSC;
            int tr, tg, tb;
            if (m - lo < 1) { tr = tg = tb = (int)base; }
            else {
                float k1 = base * (1.0f - P39_SAT);
                float k2 = base * P39_SAT / (float)(m - lo);
                tr = (int)(k1 + k2 * (pr - lo));
                tg = (int)(k1 + k2 * (pg - lo));
                tb = (int)(k1 + k2 * (pb - lo));
            }
            if (tr > 4095) tr = 4095; if (tg > 4095) tg = 4095; if (tb > 4095) tb = 4095;
            int vr = p39_lin[tr], vg = p39_lin[tg], vb = p39_lin[tb];
            vr += ((255 - vr) * bg[x * 3 + 0]) >> 8;
            vg += ((255 - vg) * bg[x * 3 + 1]) >> 8;
            vb += ((255 - vb) * bg[x * 3 + 2]) >> 8;
            out[x] = 0xFF000000u | ((uint32_t)p39_gam[vr] << 16)
                   | ((uint32_t)p39_gam[vg] << 8) | p39_gam[vb];
        }
    }
}

static uint32_t p39_lerp(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8) & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p39_utab(int w, int h) {
    for (int x = 0; x < w && x < P39_MAXD; x++) {
        long f = ((long)(2 * x + 1) * P39_AW << 15) / w - (1L << 15);
        if (f < 0) f = 0;
        long mx = ((long)(P39_AW - 1)) << 16; if (f > mx) f = mx;
        p39_x0[x] = (int32_t)(f >> 16);
        p39_fx[x] = (uint8_t)((f >> 8) & 255);
    }
    for (int y = 0; y < h && y < P39_MAXD; y++) {
        long f = ((long)(2 * y + 1) * P39_AH << 15) / h - (1L << 15);
        if (f < 0) f = 0;
        long my = ((long)(P39_AH - 1)) << 16; if (f > my) f = my;
        p39_y0[y] = (int32_t)(f >> 16);
        p39_fy[y] = (uint8_t)((f >> 8) & 255);
    }
    p39_uw = w; p39_uh = h;
}

static void p39_upscale(uint32_t *fb, int w, int h) {
    static uint32_t row[P39_AW];
    if (w > P39_MAXD || h > P39_MAXD) {          /* fallback: nearest */
        for (int y = 0; y < h; y++) {
            const uint32_t *s = p39_low + (y * P39_AH / h) * P39_AW;
            uint32_t *d = fb + (long)y * w;
            for (int x = 0; x < w; x++) d[x] = s[x * P39_AW / w];
        }
        return;
    }
    if (w != p39_uw || h != p39_uh) p39_utab(w, h);
    for (int y = 0; y < h; y++) {
        int y0 = p39_y0[y], fy = p39_fy[y];
        int y1 = (y0 < P39_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p39_low + y0 * P39_AW, *r1 = p39_low + y1 * P39_AW;
        if (fy) for (int i = 0; i < P39_AW; i++) row[i] = p39_lerp(r0[i], r1[i], fy);
        else    memcpy(row, r0, sizeof row);
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int x0 = p39_x0[x];
            int x1 = (x0 < P39_AW - 1) ? x0 + 1 : x0;
            d[x] = 0xFF000000u | p39_lerp(row[x0], row[x1], p39_fx[x]);
        }
    }
}

void pattern_039(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p39_ready) p39_tabs();
    if (sl < 2 || sl != p39_lastsl + 1) {
        memset(p39_aw, 0, sizeof p39_aw);
        memset(p39_ah, 0, sizeof p39_ah);
        p39_n = (double)(seed & 2047u);
        for (int f = 0; f < P39_BASE; f++) {
            float wg = expf(-(float)(P39_BASE - f) / P39_TAU);
            for (int i = 0; i < P39_SUB; i++) {
                p39_emit(p39_n + (double)i / P39_SUB, wg);
            }
            p39_n += 1.0;
        }
    } else {
        const float K = 0.99762183f;               /* exp(-1/420) */
        for (int i = 0; i < P39_AN; i++) { p39_aw[i] *= K; p39_ah[i] *= K; }
        for (int i = 0; i < P39_SUB; i++)
            p39_emit(p39_n + (double)i / P39_SUB, 1.0f);
        p39_n += 1.0;
    }
    p39_lastsl = sl;
    p39_compose(pal, 0.0f);
    p39_upscale(fb, w, h);
}
