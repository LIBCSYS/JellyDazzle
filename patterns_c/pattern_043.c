/* pattern_043 — Turing Labyrinth
 * Port of lab/patterns/043_turing_labyrinth/proto.py
 * Iterated DoG band-pass + saturation on 4-fold symmetric noise self-organizes
 * into a fingerprint labyrinth (built once at init, 320x240). Per frame the
 * field is blended with a soft copy (contrast breathing), colored through the
 * palette with bright seams, and bilinearly upscaled. */
#include "../jellydazzle.h"
#include <math.h>

#define P43_LW 320
#define P43_LH 240
#define P43_N  (P43_LW * P43_LH)
#define P43_IT 70

static float    p43_f0[P43_N], p43_f1[P43_N], p43_f2[P43_N], p43_f3[P43_N];
static int8_t   p43_u[P43_N];     /* labyrinth field, Q7 in [-1,1] */
static int8_t   p43_ub[P43_N];    /* softened copy                 */
static uint32_t p43_low[P43_N];
static int      p43_ready = 0;

static uint32_t p43_rs = 0x2A43C0DEu;
static float p43_gauss(void) {   /* cheap CLT-ish normal */
    float s = 0.0f;
    for (int i = 0; i < 4; i++) {
        p43_rs = p43_rs * 1664525u + 1013904223u;
        s += (float)((p43_rs >> 8) & 0xFFFF) * (1.0f / 65535.0f) - 0.5f;
    }
    return s * 1.7f;
}

static void p43_box3(const float *src, float *dst, float *tmp) {
    for (int y = 0; y < P43_LH; y++) {
        const float *a = src + (long)((y == 0) ? P43_LH - 1 : y - 1) * P43_LW;
        const float *b = src + (long)y * P43_LW;
        const float *c = src + (long)((y == P43_LH - 1) ? 0 : y + 1) * P43_LW;
        float *o = tmp + (long)y * P43_LW;
        for (int x = 0; x < P43_LW; x++) o[x] = (a[x] + b[x] + c[x]) * (1.0f / 3.0f);
    }
    for (int y = 0; y < P43_LH; y++) {
        const float *r = tmp + (long)y * P43_LW;
        float *o = dst + (long)y * P43_LW;
        o[0] = (r[P43_LW - 1] + r[0] + r[1]) * (1.0f / 3.0f);
        for (int x = 1; x < P43_LW - 1; x++) o[x] = (r[x - 1] + r[x] + r[x + 1]) * (1.0f / 3.0f);
        o[P43_LW - 1] = (r[P43_LW - 2] + r[P43_LW - 1] + r[0]) * (1.0f / 3.0f);
    }
}

static void p43_build(void) {
    /* 4-fold mirror symmetric noise seed */
    for (int y = 0; y < P43_LH / 2; y++)
        for (int x = 0; x < P43_LW / 2; x++) {
            float v = p43_gauss();
            p43_f0[y * P43_LW + x] = v;
            p43_f0[y * P43_LW + (P43_LW - 1 - x)] = v;
            p43_f0[(P43_LH - 1 - y) * P43_LW + x] = v;
            p43_f0[(P43_LH - 1 - y) * P43_LW + (P43_LW - 1 - x)] = v;
        }
    for (int it = 0; it < P43_IT; it++) {
        p43_box3(p43_f0, p43_f1, p43_f3);
        p43_box3(p43_f1, p43_f1, p43_f3);          /* sb (small blur) */
        p43_box3(p43_f1, p43_f2, p43_f3);
        for (int k = 0; k < 5; k++) p43_box3(p43_f2, p43_f2, p43_f3);   /* lb */
        for (int i = 0; i < P43_N; i++) {
            float v = (p43_f1[i] - p43_f2[i]) * 12.0f;
            if (v > 1.0f) v = 1.0f; else if (v < -1.0f) v = -1.0f;
            p43_f0[i] = v;
        }
    }
    p43_box3(p43_f0, p43_f0, p43_f3);          /* soften the hard clip a touch */
    for (int i = 0; i < P43_N; i++) p43_f0[i] *= 1.35f;
    p43_box3(p43_f0, p43_f1, p43_f3);
    p43_box3(p43_f1, p43_f1, p43_f3);
    p43_box3(p43_f1, p43_f1, p43_f3);
    for (int i = 0; i < P43_N; i++) {
        int a = (int)lrintf(p43_f0[i] * 127.0f);
        int b = (int)lrintf(p43_f1[i] * 127.0f);
        p43_u[i]  = (int8_t)(a > 127 ? 127 : a < -127 ? -127 : a);
        p43_ub[i] = (int8_t)(b > 127 ? 127 : b < -127 ? -127 : b);
    }
    p43_ready = 1;
}

static inline uint32_t p43_lerp(uint32_t a, uint32_t b, unsigned t) {
    unsigned s = 256u - t;
    uint32_t rb = (((a & 0xFF00FFu) * s + (b & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
    uint32_t g  = (((a & 0x00FF00u) * s + (b & 0x00FF00u) * t) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p43_blit(uint32_t *fb, int w, int h) {
    for (int y = 0; y < h; y++) {
        long fy = (((long)(2 * y + 1) * P43_LH) << 15) / h - (1L << 15);
        if (fy < 0) fy = 0;
        long fym = ((long)(P43_LH - 1)) << 16;
        if (fy > fym) fy = fym;
        int y0 = (int)(fy >> 16), y1 = y0 + 1 < P43_LH ? y0 + 1 : y0;
        unsigned wy = (unsigned)((fy >> 8) & 255);
        const uint32_t *r0 = p43_low + (long)y0 * P43_LW;
        const uint32_t *r1 = p43_low + (long)y1 * P43_LW;
        uint32_t *dst = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            long fx = (((long)(2 * x + 1) * P43_LW) << 15) / w - (1L << 15);
            if (fx < 0) fx = 0;
            long fxm = ((long)(P43_LW - 1)) << 16;
            if (fx > fxm) fx = fxm;
            int x0 = (int)(fx >> 16), x1 = x0 + 1 < P43_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((fx >> 8) & 255);
            uint32_t t = p43_lerp(r0[x0], r0[x1], wx);
            uint32_t b = p43_lerp(r1[x0], r1[x1], wx);
            dst[x] = 0xFF000000u | p43_lerp(t, b, wy);
        }
    }
}

void pattern_043(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!p43_ready) p43_build();
    float t = (float)frame;
    /* contrast breathing: uu = u*(1-0.35m) + ub*0.35m */
    float m = 0.5f + 0.5f * sinf(t * 0.006f);
    unsigned mb = (unsigned)(0.35f * m * 256.0f);       /* blend weight 0..90 */
    int drift = (int)(t * 0.00028f * 32768.0f) + (int)(seed & 32767u);

    /* per-pixel tables over uu (-127..127 -> 0..254) */
    static uint8_t bri[256], seam[256], idxo[256];
    for (int i = 0; i < 256; i++) {
        float uu = ((float)i - 127.0f) / 127.0f;
        if (uu < -1.0f) uu = -1.0f; else if (uu > 1.0f) uu = 1.0f;
        float v = 0.5f + 0.5f * uu;
        float r = 1.0f - fabsf(uu); r = r * r;
        bri[i]  = (uint8_t)(255.0f * (0.30f + 0.70f * v));
        seam[i] = (uint8_t)(90.0f * r);
        idxo[i] = (uint8_t)(255.0f * v);
    }
    for (int i = 0; i < P43_N; i++) {
        int uu = (int)p43_u[i] + (((int)p43_ub[i] - (int)p43_u[i]) * (int)mb >> 8);
        int q = uu + 127;
        if (q < 0) q = 0; else if (q > 255) q = 255;
        uint32_t c = pal[(drift + ((idxo[q] * 7000) >> 8)) & JD_PAL_MASK];
        unsigned g = bri[q], s = seam[q];
        unsigned rr = ((((c >> 16) & 255) * g) >> 8) + s;
        unsigned gg = ((((c >> 8) & 255) * g) >> 8) + s;
        unsigned bb = (((c & 255) * g) >> 8) + s;
        if (rr > 255) rr = 255;
        if (gg > 255) gg = 255;
        if (bb > 255) bb = 255;
        p43_low[i] = (rr << 16) | (gg << 8) | bb;
    }
    p43_blit(fb, w, h);
}
