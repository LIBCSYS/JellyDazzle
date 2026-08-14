/* 041 cyclic_bloom — cyclic cellular automaton, kaleidoscopic seed, rainbow states.
 * Live CA (160x120, K=12) reseeded per segment, scrubbed with smoothstep
 * crossfade between adjacent generations; bilinear upscale to fb. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define P41_GW 160
#define P41_GH 120
#define P41_K  12
#define P41_SKIP 120
#define P41_SPEED 0.07f          /* generations per frame (proto 0.15, eased for motion law) */

static uint8_t p41_ga[P41_GH * P41_GW];   /* generation i   */
static uint8_t p41_gb[P41_GH * P41_GW];   /* generation i+1 */
static uint8_t p41_gt[P41_GH * P41_GW];
static uint32_t p41_cbuf[P41_GH * P41_GW];
static int p41_inited = 0;
static int p41_last_sl = -1;
static uint32_t p41_seed_used = 0;
static int p41_curgen = 0;

static uint32_t p41_rs;
static uint32_t p41_rnd(void) {
    p41_rs = p41_rs * 1664525u + 1013904223u;
    return p41_rs >> 16;
}

static inline uint32_t p41_lerp(uint32_t a, uint32_t b, unsigned t) {
    unsigned s = 256u - t;
    uint32_t rb = (((a & 0xFF00FFu) * s + (b & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
    uint32_t g  = (((a & 0x00FF00u) * s + (b & 0x00FF00u) * t) >> 8) & 0x00FF00u;
    return 0xFF000000u | rb | g;
}

/* one CA generation: cell adopts (s+1)%K if any Moore neighbor holds it */
static void p41_step(const uint8_t *src, uint8_t *dst) {
    static uint8_t inc[P41_K];
    for (int i = 0; i < P41_K; i++) inc[i] = (uint8_t)((i + 1) % P41_K);
    for (int y = 0; y < P41_GH; y++) {
        int ym = (y == 0 ? P41_GH - 1 : y - 1) * P41_GW;
        int yc = y * P41_GW;
        int yp = (y == P41_GH - 1 ? 0 : y + 1) * P41_GW;
        for (int x = 0; x < P41_GW; x++) {
            int xm = (x == 0 ? P41_GW - 1 : x - 1);
            int xp = (x == P41_GW - 1 ? 0 : x + 1);
            uint8_t s = src[yc + x];
            uint8_t n = inc[s];
            int hit = (src[ym + xm] == n) | (src[ym + x] == n) | (src[ym + xp] == n)
                    | (src[yc + xm] == n)                      | (src[yc + xp] == n)
                    | (src[yp + xm] == n) | (src[yp + x] == n) | (src[yp + xp] == n);
            dst[yc + x] = hit ? n : s;
        }
    }
}

static void p41_reseed(uint32_t seed) {
    p41_rs = seed ^ 0x9E3779B9u;
    for (int i = 0; i < 8; i++) p41_rnd();
    for (int y = 0; y < P41_GH / 2; y++)
        for (int x = 0; x < P41_GW / 2; x++) {
            uint8_t v = (uint8_t)(p41_rnd() % P41_K);
            p41_ga[y * P41_GW + x] = v;
            p41_ga[y * P41_GW + (P41_GW - 1 - x)] = v;
            p41_ga[(P41_GH - 1 - y) * P41_GW + x] = v;
            p41_ga[(P41_GH - 1 - y) * P41_GW + (P41_GW - 1 - x)] = v;
        }
    for (int g = 0; g < P41_SKIP; g++) {
        p41_step(p41_ga, p41_gt);
        memcpy(p41_ga, p41_gt, sizeof p41_ga);
    }
    p41_step(p41_ga, p41_gb);
    p41_curgen = 0;
    p41_seed_used = seed;
    p41_inited = 1;
}

/* bilinear upscale of small color buffer to the framebuffer */
static int p41_uw = -1, p41_usw = -1;
static int p41_x0t[4096], p41_x1t[4096];
static uint8_t p41_wxt[4096];
static uint32_t p41_rowbuf[512];
static void p41_upscale(const uint32_t *src, int sw, int sh, uint32_t *fb, int w, int h) {
    if (w != p41_uw || sw != p41_usw) {
        int n = w > 4096 ? 4096 : w;
        for (int x = 0; x < n; x++) {
            long fx = (((long)(2 * x + 1) * sw) << 15) / w - 32768;
            if (fx < 0) fx = 0;
            int x0 = (int)(fx >> 16);
            if (x0 > sw - 1) x0 = sw - 1;
            int x1 = x0 + 1 < sw ? x0 + 1 : sw - 1;
            p41_x0t[x] = x0; p41_x1t[x] = x1; p41_wxt[x] = (uint8_t)((fx >> 8) & 255);
        }
        p41_uw = w; p41_usw = sw;
    }
    for (int y = 0; y < h; y++) {
        long fy = (((long)(2 * y + 1) * sh) << 15) / h - 32768;
        if (fy < 0) fy = 0;
        int y0 = (int)(fy >> 16);
        if (y0 > sh - 1) y0 = sh - 1;
        int y1 = y0 + 1 < sh ? y0 + 1 : sh - 1;
        unsigned wy = (unsigned)((fy >> 8) & 255);
        const uint32_t *r0 = src + (long)y0 * sw, *r1 = src + (long)y1 * sw;
        for (int x = 0; x < sw; x++) p41_rowbuf[x] = p41_lerp(r0[x], r1[x], wy);
        uint32_t *out = fb + (long)y * w;
        if (w <= 4096) {
            for (int x = 0; x < w; x++)
                out[x] = p41_lerp(p41_rowbuf[p41_x0t[x]], p41_rowbuf[p41_x1t[x]], p41_wxt[x]);
        } else {
            for (int x = 0; x < w; x++) {
                int sxn = (int)(((long)x * sw) / w);
                out[x] = p41_rowbuf[sxn];
            }
        }
    }
}

void pattern_041(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p41_inited || sl == 0 || sl < p41_last_sl || seed != p41_seed_used)
        p41_reseed(seed);
    p41_last_sl = sl;

    float pos = (float)sl * P41_SPEED;
    int gi = (int)pos;
    while (p41_curgen < gi) {           /* advance live CA to generation gi */
        memcpy(p41_ga, p41_gb, sizeof p41_ga);
        p41_step(p41_ga, p41_gb);
        p41_curgen++;
    }
    float f = pos - (float)gi;
    f = f * f * (3.0f - 2.0f * f);
    unsigned fw = (unsigned)(f * 256.0f);
    if (fw > 256u) fw = 256u;

    int drift = (int)((float)sl * 0.0006f * 32768.0f);
    uint32_t colA[P41_K], colB[P41_K], pair[P41_K][P41_K];
    for (int s = 0; s < P41_K; s++) {
        int idx = ((s * 32768) / P41_K + drift) & JD_PAL_MASK;
        colA[s] = pal[idx] | 0xFF000000u;
        colB[s] = colA[s];
    }
    for (int a = 0; a < P41_K; a++)
        for (int b = 0; b < P41_K; b++)
            pair[a][b] = p41_lerp(colA[a], colB[b], fw);

    for (int i = 0; i < P41_GH * P41_GW; i++)
        p41_cbuf[i] = pair[p41_ga[i]][p41_gb[i]];

    p41_upscale(p41_cbuf, P41_GW, P41_GH, fb, w, h);
}
