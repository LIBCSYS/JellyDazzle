/* pattern_048 — Excitable Spirals
 * Port of lab/patterns/048_excitable_spirals/proto.py
 * Greenberg-Hastings excitable medium (160x120, K=8) seeded with three broken
 * wavefront bars mirrored 4-fold; wavefronts crawl and curl into greek-key
 * spiral cores. Run live (1 generation per 20 frames) with a smoothstep
 * crossfade between consecutive generations, then bilinear upscale. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define P48_GW 160
#define P48_GH 120
#define P48_N  (P48_GW * P48_GH)
#define P48_K  8
#define P48_SKIP 100
#define P48_SPEED 0.05f

static uint8_t  p48_a[P48_N];      /* generation g   */
static uint8_t  p48_b[P48_N];      /* generation g+1 */
static uint8_t  p48_t[P48_N];
static uint32_t p48_low[P48_N];
static int      p48_ready = 0;
static int      p48_gen = 0;

static void p48_step(const uint8_t *src, uint8_t *dst) {
    for (int y = 0; y < P48_GH; y++) {
        int ym = ((y == 0) ? P48_GH - 1 : y - 1) * P48_GW;
        int yc = y * P48_GW;
        int yp = ((y == P48_GH - 1) ? 0 : y + 1) * P48_GW;
        for (int x = 0; x < P48_GW; x++) {
            int xm = (x == 0) ? P48_GW - 1 : x - 1;
            int xp = (x == P48_GW - 1) ? 0 : x + 1;
            uint8_t s = src[yc + x];
            if (s == 0) {
                int n = (src[ym + xm] == 1) | (src[ym + x] == 1) | (src[ym + xp] == 1)
                      | (src[yc + xm] == 1) | (src[yc + xp] == 1)
                      | (src[yp + xm] == 1) | (src[yp + x] == 1) | (src[yp + xp] == 1);
                dst[yc + x] = (uint8_t)(n ? 1 : 0);
            } else {
                dst[yc + x] = (uint8_t)((s + 1) & (P48_K - 1));
            }
        }
    }
}

static void p48_seed(void) {
    static const int bars[3][4] = { {14, 20, 58, 1}, {34, 8, 46, -1}, {50, 28, 70, 1} };
    static uint8_t q[60 * 80];
    memset(q, 0, sizeof q);
    for (int b = 0; b < 3; b++) {
        int y = bars[b][0], x0 = bars[b][1], x1 = bars[b][2], side = bars[b][3];
        for (int x = x0; x < x1; x++) {
            q[y * 80 + x] = 1;
            q[(y + side) * 80 + x] = P48_K / 2;
        }
    }
    for (int y = 0; y < 60; y++)
        for (int x = 0; x < 80; x++) {
            uint8_t v = q[y * 80 + x];
            p48_a[y * P48_GW + x] = v;
            p48_a[y * P48_GW + (P48_GW - 1 - x)] = v;
            p48_a[(P48_GH - 1 - y) * P48_GW + x] = v;
            p48_a[(P48_GH - 1 - y) * P48_GW + (P48_GW - 1 - x)] = v;
        }
    for (int g = 0; g < P48_SKIP; g++) {
        p48_step(p48_a, p48_t);
        memcpy(p48_a, p48_t, sizeof p48_a);
    }
    p48_step(p48_a, p48_b);
    p48_gen = 0;
    p48_ready = 1;
}

static inline uint32_t p48_lerp(uint32_t a, uint32_t b, unsigned t) {
    unsigned s = 256u - t;
    uint32_t rb = (((a & 0xFF00FFu) * s + (b & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
    uint32_t g  = (((a & 0x00FF00u) * s + (b & 0x00FF00u) * t) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p48_blit(uint32_t *fb, int w, int h) {
    for (int y = 0; y < h; y++) {
        long fy = (((long)(2 * y + 1) * P48_GH) << 15) / h - (1L << 15);
        if (fy < 0) fy = 0;
        long fym = ((long)(P48_GH - 1)) << 16;
        if (fy > fym) fy = fym;
        int y0 = (int)(fy >> 16), y1 = y0 + 1 < P48_GH ? y0 + 1 : y0;
        unsigned wy = (unsigned)((fy >> 8) & 255);
        const uint32_t *r0 = p48_low + (long)y0 * P48_GW;
        const uint32_t *r1 = p48_low + (long)y1 * P48_GW;
        uint32_t *dst = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            long fx = (((long)(2 * x + 1) * P48_GW) << 15) / w - (1L << 15);
            if (fx < 0) fx = 0;
            long fxm = ((long)(P48_GW - 1)) << 16;
            if (fx > fxm) fx = fxm;
            int x0 = (int)(fx >> 16), x1 = x0 + 1 < P48_GW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((fx >> 8) & 255);
            uint32_t t = p48_lerp(r0[x0], r0[x1], wx);
            uint32_t b = p48_lerp(r1[x0], r1[x1], wx);
            dst[x] = 0xFF000000u | p48_lerp(t, b, wy);
        }
    }
}

void pattern_048(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!p48_ready) p48_seed();
    float t = (float)frame;
    float pos = t * P48_SPEED;
    int gi = (int)pos;
    if (gi < p48_gen || gi - p48_gen > 512) p48_seed();   /* out-of-order restart */
    while (p48_gen < gi) {
        memcpy(p48_a, p48_b, sizeof p48_a);
        p48_step(p48_a, p48_b);
        p48_gen++;
    }
    float f = pos - (float)gi;
    f = f * f * (3.0f - 2.0f * f);
    unsigned fw = (unsigned)(f * 256.0f);
    if (fw > 256u) fw = 256u;

    int drift = (int)(t * 0.00028f * 32768.0f) + (int)(seed & 32767u);
    uint32_t st[P48_K], pair[P48_K][P48_K];
    for (int s = 0; s < P48_K; s++) {
        if (s == 0) {
            st[s] = 0x00080A16u;                       /* resting ground */
        } else {
            float age = (float)(s - 1) / (float)(P48_K - 1);
            float br = 0.22f + 0.78f * powf(1.0f - age, 1.2f);
            uint32_t c = pal[(drift + (int)(age * 3400.0f)) & JD_PAL_MASK];
            unsigned g = (unsigned)(br * 300.0f);
            float e = 1.0f - age;
            unsigned hi = (unsigned)(95.0f * e * e * e);   /* white-hot leading edge */
            unsigned rr = 8u + hi + ((((c >> 16) & 255) * g) >> 8);
            unsigned gg = 10u + hi + ((((c >> 8) & 255) * g) >> 8);
            unsigned bb = 22u + hi + (((c & 255) * g) >> 8);
            if (rr > 255) rr = 255;
            if (gg > 255) gg = 255;
            if (bb > 255) bb = 255;
            st[s] = (rr << 16) | (gg << 8) | bb;
        }
    }
    for (int a = 0; a < P48_K; a++)
        for (int b = 0; b < P48_K; b++)
            pair[a][b] = p48_lerp(st[a], st[b], fw);
    for (int i = 0; i < P48_N; i++)
        p48_low[i] = pair[p48_a[i]][p48_b[i]];
    p48_blit(fb, w, h);
}
