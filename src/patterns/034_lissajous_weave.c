/* pattern_034 — Lissajous Weave
 * Port of lab/patterns/034_lissajous_weave/proto.py
 * A 3:4 Lissajous figure with a sub-detuned y term (4.0015) so it never
 * closes, plus a slow phase sweep on x that reads as a 3-D tumble.  Mirrored
 * left/right into a 320x240 weight accumulator with exponential age fade;
 * blurred, tone-mapped through 1-exp(-a*gain), vignette-blended and
 * bilinearly upscaled.  A parallel weight*hue accumulator carries the slowly
 * crawling palette position.  Accumulator pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <string.h>

#define P34_AW   320
#define P34_AH   240
#define P34_AN   (P34_AW * P34_AH)
#define P34_MAXD 4096
#define P34_SUB  44
#define P34_BASE 300
#define P34_TAU  450.0f
#define P34_GAIN 0.30f
#define P34_SAT  0.95f        /* proto HSV saturation */
#define P34_TSC  585.14f          /* 4096/7 : tone LUT index scale */

static float    p34_aw[P34_AN];   /* weight accumulator      */
static float    p34_ah[P34_AN];   /* weight * hue            */
static float    p34_bw[P34_AN];   /* h-blurred weight        */
static float    p34_bh[P34_AN];   /* h-blurred weight*hue    */
static uint32_t p34_low[P34_AN];
static uint8_t  p34_bg[P34_AN * 3];
static uint8_t  p34_lin[4096];
static uint8_t  p34_gam[256];
static int      p34_ready = 0;
static int      p34_lastsl = -1000;
static double   p34_n = 0.0;
static int      p34_uw = -1, p34_uh = -1;
static int32_t  p34_x0[P34_MAXD], p34_y0[P34_MAXD];
static uint8_t  p34_fx[P34_MAXD], p34_fy[P34_MAXD];

static void p34_tabs(void) {
    for (int i = 0; i < 4096; i++)
        p34_lin[i] = (uint8_t)((1.0 - exp(-(double)i * 7.0 / 4096.0)) * 255.0 + 0.5);
    for (int i = 0; i < 256; i++)
        p34_gam[i] = (uint8_t)(pow((double)i / 255.0, 0.85) * 255.0 + 0.5);
    for (int y = 0; y < P34_AH; y++)
        for (int x = 0; x < P34_AW; x++) {
            double dx = (x - P34_AW * 0.5) / P34_AW;
            double dy = (y - P34_AH * 0.5) / P34_AH;
            double e = exp(-(dx * dx + dy * dy) * 3.0) * 255.0;
            int i = (y * P34_AW + x) * 3;
            p34_bg[i + 0] = (uint8_t)(e * 0.02 + 0.5);
            p34_bg[i + 1] = (uint8_t)(e * 0.02 + 0.5);
            p34_bg[i + 2] = (uint8_t)(e * 0.05 + 0.5);
        }
    p34_ready = 1;
}

static void p34_splat(int xi, int yi, float w, float hw) {
    if ((unsigned)xi >= (unsigned)P34_AW || (unsigned)yi >= (unsigned)P34_AH) return;
    int i = yi * P34_AW + xi;
    p34_aw[i] += w;
    p34_ah[i] += hw;
}

static void p34_emit(double n, float w) {
    double sp = n * 0.17;
    double dl = n * 0.0013;                    /* phase sweep = 3-D tumble */
    float  x  = (float)(118.0 * sin(3.0 * sp + dl));
    float  y  = (float)(88.0 * sin(4.0015 * sp));
    /* hue crawls with arc length, damped so the whole visible window
       reads as one scheme (the global drift is added at compose time) */
    float  hw = (float)(sp * 0.0021 * 0.30) * w;
    const float ox = P34_AW * 0.5f, oy = P34_AH * 0.5f;
    int ya = (int)lrintf(oy + y);
    p34_splat((int)lrintf(ox + x), ya, w, hw);
    p34_splat((int)lrintf(ox - x), ya, w, hw);
}

static void p34_hblur(void) {
    for (int y = 0; y < P34_AH; y++) {
        const float *rw = p34_aw + y * P34_AW, *rh = p34_ah + y * P34_AW;
        float *ow = p34_bw + y * P34_AW, *oh = p34_bh + y * P34_AW;
        ow[0] = 0.75f * rw[0] + 0.25f * rw[1];
        oh[0] = 0.75f * rh[0] + 0.25f * rh[1];
        for (int x = 1; x < P34_AW - 1; x++) {
            ow[x] = 0.5f * rw[x] + 0.25f * (rw[x - 1] + rw[x + 1]);
            oh[x] = 0.5f * rh[x] + 0.25f * (rh[x - 1] + rh[x + 1]);
        }
        ow[P34_AW - 1] = 0.75f * rw[P34_AW - 1] + 0.25f * rw[P34_AW - 2];
        oh[P34_AW - 1] = 0.75f * rh[P34_AW - 1] + 0.25f * rh[P34_AW - 2];
    }
}

static void p34_compose(const uint32_t *pal, float hoff) {
    p34_hblur();
    for (int y = 0; y < P34_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P34_AH - 1) ? y + 1 : y;
        const float *cw = p34_bw + y * P34_AW, *uw = p34_bw + yu * P34_AW,
                    *dw = p34_bw + yd * P34_AW;
        const float *ch = p34_bh + y * P34_AW, *uh = p34_bh + yu * P34_AW,
                    *dh = p34_bh + yd * P34_AW;
        uint32_t *out = p34_low + y * P34_AW;
        const uint8_t *bg = p34_bg + y * P34_AW * 3;
        for (int x = 0; x < P34_AW; x++) {
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
            float base = a * P34_GAIN * P34_TSC;
            int tr, tg, tb;
            if (m - lo < 1) { tr = tg = tb = (int)base; }
            else {
                float k1 = base * (1.0f - P34_SAT);
                float k2 = base * P34_SAT / (float)(m - lo);
                tr = (int)(k1 + k2 * (pr - lo));
                tg = (int)(k1 + k2 * (pg - lo));
                tb = (int)(k1 + k2 * (pb - lo));
            }
            if (tr > 4095) tr = 4095; if (tg > 4095) tg = 4095; if (tb > 4095) tb = 4095;
            int vr = p34_lin[tr], vg = p34_lin[tg], vb = p34_lin[tb];
            vr += ((255 - vr) * bg[x * 3 + 0]) >> 8;
            vg += ((255 - vg) * bg[x * 3 + 1]) >> 8;
            vb += ((255 - vb) * bg[x * 3 + 2]) >> 8;
            out[x] = 0xFF000000u | ((uint32_t)p34_gam[vr] << 16)
                   | ((uint32_t)p34_gam[vg] << 8) | p34_gam[vb];
        }
    }
}

static uint32_t p34_lerp(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8) & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p34_utab(int w, int h) {
    for (int x = 0; x < w && x < P34_MAXD; x++) {
        long f = ((long)(2 * x + 1) * P34_AW << 15) / w - (1L << 15);
        if (f < 0) f = 0;
        long mx = ((long)(P34_AW - 1)) << 16; if (f > mx) f = mx;
        p34_x0[x] = (int32_t)(f >> 16);
        p34_fx[x] = (uint8_t)((f >> 8) & 255);
    }
    for (int y = 0; y < h && y < P34_MAXD; y++) {
        long f = ((long)(2 * y + 1) * P34_AH << 15) / h - (1L << 15);
        if (f < 0) f = 0;
        long my = ((long)(P34_AH - 1)) << 16; if (f > my) f = my;
        p34_y0[y] = (int32_t)(f >> 16);
        p34_fy[y] = (uint8_t)((f >> 8) & 255);
    }
    p34_uw = w; p34_uh = h;
}

static void p34_upscale(uint32_t *fb, int w, int h) {
    static uint32_t row[P34_AW];
    if (w > P34_MAXD || h > P34_MAXD) {          /* fallback: nearest */
        for (int y = 0; y < h; y++) {
            const uint32_t *s = p34_low + (y * P34_AH / h) * P34_AW;
            uint32_t *d = fb + (long)y * w;
            for (int x = 0; x < w; x++) d[x] = s[x * P34_AW / w];
        }
        return;
    }
    if (w != p34_uw || h != p34_uh) p34_utab(w, h);
    for (int y = 0; y < h; y++) {
        int y0 = p34_y0[y], fy = p34_fy[y];
        int y1 = (y0 < P34_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p34_low + y0 * P34_AW, *r1 = p34_low + y1 * P34_AW;
        if (fy) for (int i = 0; i < P34_AW; i++) row[i] = p34_lerp(r0[i], r1[i], fy);
        else    memcpy(row, r0, sizeof row);
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int x0 = p34_x0[x];
            int x1 = (x0 < P34_AW - 1) ? x0 + 1 : x0;
            d[x] = 0xFF000000u | p34_lerp(row[x0], row[x1], p34_fx[x]);
        }
    }
}

void pattern_034(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p34_ready) p34_tabs();
    if (sl < 2 || sl != p34_lastsl + 1) {
        memset(p34_aw, 0, sizeof p34_aw);
        memset(p34_ah, 0, sizeof p34_ah);
        p34_n = (double)(seed & 2047u);
        for (int f = 0; f < P34_BASE; f++) {
            float wg = expf(-(float)(P34_BASE - f) / P34_TAU);
            for (int i = 0; i < P34_SUB; i++) {
                p34_emit(p34_n + (double)i / P34_SUB, wg);
            }
            p34_n += 1.0;
        }
    } else {
        const float K = 0.99778025f;               /* exp(-1/450) */
        for (int i = 0; i < P34_AN; i++) { p34_aw[i] *= K; p34_ah[i] *= K; }
        for (int i = 0; i < P34_SUB; i++)
            p34_emit(p34_n + (double)i / P34_SUB, 1.0f);
        p34_n += 1.0;
    }
    p34_lastsl = sl;
    p34_compose(pal, (float)(p34_n * 0.0009));
    p34_upscale(fb, w, h);
}
