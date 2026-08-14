/* pattern_031 — Spirograph Bloom
 * Port of lab/patterns/031_spirograph_bloom/proto.py
 * Hypotrochoid with breathing gear ratio, D6 (6 rot x mirror) symmetry,
 * accumulated at 320x240 with exponential age fade, tone-mapped and
 * bilinearly upscaled to the framebuffer. Hue maps to the cyclic palette. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define P31_AW   320
#define P31_AH   240
#define P31_N    (P31_AW * P31_AH)
#define P31_BASE 260
#define P31_SUB  36
#define P31_TAU  380.0f
#define P31_GAIN 0.16f
#define P31_TWOPI 6.283185307179586f

static float    p31_aw[P31_N];   /* weight accumulator            */
static float    p31_hs[P31_N];   /* weight * sin(2*pi*hue)        */
static float    p31_hc[P31_N];   /* weight * cos(2*pi*hue)        */
static float    p31_bl[P31_N];   /* h-blurred copy of p31_aw      */
static uint32_t p31_low[P31_N];  /* composed low-res ARGB         */
static uint8_t  p31_tone[4096];
static uint8_t  p31_vig[P31_N];
static float    p31_secc[6], p31_secs[6];
static int      p31_ready = 0;
static int      p31_last_sl = -2;
static double   p31_n = 0.0;     /* substep counter               */

static void p31_static_init(void) {
    for (int i = 0; i < 4096; i++) {
        double v = 1.0 - exp(-(double)i * 7.0 / 4096.0);
        p31_tone[i] = (uint8_t)(pow(v, 0.85) * 255.0 + 0.5);
    }
    for (int y = 0; y < P31_AH; y++)
        for (int x = 0; x < P31_AW; x++) {
            double dx = (x - P31_AW * 0.5) / P31_AW;
            double dy = (y - P31_AH * 0.5) / P31_AH;
            p31_vig[y * P31_AW + x] =
                (uint8_t)(exp(-(dx * dx + dy * dy) * 3.0) * 255.0);
        }
    for (int k = 0; k < 6; k++) {
        double a = k * (3.141592653589793 / 3.0);
        p31_secc[k] = (float)cos(a);
        p31_secs[k] = (float)sin(a);
    }
    p31_ready = 1;
}

static void p31_splat(float x, float y, float s, float c, float w) {
    int xi = (int)floorf(x + 0.5f), yi = (int)floorf(y + 0.5f);
    if (xi < 0 || xi >= P31_AW || yi < 0 || yi >= P31_AH) return;
    int i = yi * P31_AW + xi;
    p31_aw[i] += w;
    p31_hs[i] += w * s;
    p31_hc[i] += w * c;
}

static void p31_emit(double n, float w) {
    double th = n * 0.11;
    double q  = 2.5 + 1.2 * sin(n * 0.0021);
    double d  = 0.35 + 0.25 * sin(n * 0.0009 + 1.7);
    float  x  = (float)(55.2 * cos(th) + 92.0 * d * cos(q * th));
    float  y  = (float)(55.2 * sin(th) - 92.0 * d * sin(q * th));
    float  hu = (float)(th * 0.0028) * P31_TWOPI;
    float  hsn = sinf(hu), hcs = cosf(hu);
    for (int k = 0; k < 6; k++) {
        float ck = p31_secc[k], sk = p31_secs[k];
        for (int m = 0; m < 2; m++) {
            float yy = m ? -y : y;
            p31_splat(x * ck - yy * sk + P31_AW * 0.5f,
                      x * sk + yy * ck + P31_AH * 0.5f, hsn, hcs, w);
        }
    }
}

static void p31_compose(int tt) {
    /* horizontal 1-2-1 blur of the weight plane */
    for (int y = 0; y < P31_AH; y++) {
        const float *r = p31_aw + y * P31_AW;
        float *o = p31_bl + y * P31_AW;
        o[0] = 0.75f * r[0] + 0.25f * r[1];
        for (int x = 1; x < P31_AW - 1; x++)
            o[x] = 0.5f * r[x] + 0.25f * (r[x - 1] + r[x + 1]);
        o[P31_AW - 1] = 0.75f * r[P31_AW - 1] + 0.25f * r[P31_AW - 2];
    }
    const float tscale = 4096.0f * P31_GAIN / 7.0f;
    const float off = (float)(tt * 0.0007);              /* global hue drift */
    const float h2i = 32768.0f / P31_TWOPI;
    const int   ioff = (int)(off * 32768.0f);
    /* bg vignette color 0.04,0.01,0.07 */
    const int bgr = 10, bgg = 3, bgb = 18;
    for (int y = 0; y < P31_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P31_AH - 1) ? y + 1 : y;
        for (int x = 0; x < P31_AW; x++) {
            int i = y * P31_AW + x;
            float a = 0.5f * p31_bl[i]
                    + 0.25f * (p31_bl[yu * P31_AW + x] + p31_bl[yd * P31_AW + x]);
            int ti = (int)(a * tscale);
            if (ti > 4095) ti = 4095;
            int v8 = p31_tone[ti];
            int idx = ((int)(atan2f(p31_hs[i], p31_hc[i]) * h2i) + ioff) & 32767;
            uint32_t c = ((const uint32_t *)p31_pal)[idx];
            int r = (((c >> 16) & 255) * v8) >> 8;
            int g = (((c >> 8) & 255) * v8) >> 8;
            int b = ((c & 255) * v8) >> 8;
            if (v8 > 200) {                        /* white-hot core */
                int f = ((v8 - 200) * 255 / 55 * 200) >> 8;
                r += ((255 - r) * f) >> 8;
                g += ((255 - g) * f) >> 8;
                b += ((255 - b) * f) >> 8;
            }
            int vg = p31_vig[i];
            r += ((255 - r) * vg * bgr) >> 16;
            g += ((255 - g) * vg * bgg) >> 16;
            b += ((255 - b) * vg * bgb) >> 16;
            p31_low[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
}

static const uint32_t *p31_pal; /* set per call before compose */

static uint32_t p31_lerp2(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8)
                  & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8)
                  & 0x00FF00u;
    return rb | g;
}

static void p31_upscale(uint32_t *fb, int w, int h) {
    for (int y = 0; y < h; y++) {
        long fy = ((long)(2 * y + 1) * P31_AH << 15) / h - (1L << 15);
        if (fy < 0) fy = 0;
        long fymax = ((long)(P31_AH - 1)) << 16;
        if (fy > fymax) fy = fymax;
        int y0 = (int)(fy >> 16), wy = (int)((fy >> 8) & 255);
        int y1 = (y0 < P31_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p31_low + y0 * P31_AW;
        const uint32_t *r1 = p31_low + y1 * P31_AW;
        uint32_t *dst = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            long fx = ((long)(2 * x + 1) * P31_AW << 15) / w - (1L << 15);
            if (fx < 0) fx = 0;
            long fxmax = ((long)(P31_AW - 1)) << 16;
            if (fx > fxmax) fx = fxmax;
            int x0 = (int)(fx >> 16), wx = (int)((fx >> 8) & 255);
            int x1 = (x0 < P31_AW - 1) ? x0 + 1 : x0;
            uint32_t top = p31_lerp2(r0[x0], r0[x1], wx);
            uint32_t bot = p31_lerp2(r1[x0], r1[x1], wx);
            dst[x] = 0xFF000000u | p31_lerp2(top, bot, wy);
        }
    }
}

void pattern_031(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p31_ready) p31_static_init();
    if (sl != p31_last_sl + 1) {                    /* segment (re)start */
        memset(p31_aw, 0, sizeof p31_aw);
        memset(p31_hs, 0, sizeof p31_hs);
        memset(p31_hc, 0, sizeof p31_hc);
        p31_n = (double)(seed & 8191);
        for (int f = 0; f < P31_BASE; f++) {        /* warm start */
            float wgt = expf(-(float)(P31_BASE - f) / P31_TAU);
            for (int i = 0; i < P31_SUB; i++) { p31_emit(p31_n, wgt); p31_n += 1.0; }
        }
    } else {
        const float K = 0.99737189f;                /* exp(-1/380) */
        for (int i = 0; i < P31_N; i++) {
            p31_aw[i] *= K; p31_hs[i] *= K; p31_hc[i] *= K;
        }
        for (int i = 0; i < P31_SUB; i++) { p31_emit(p31_n, 1.0f); p31_n += 1.0; }
    }
    p31_last_sl = sl;
    p31_pal = pal;
    p31_compose(sl + P31_BASE);
    p31_upscale(fb, w, h);
}
