/* pattern_044 — Majority Quilt
 * Port of lab/patterns/044_majority_quilt/proto.py
 * 160x120 majority-vote CA on a 4-fold symmetric +-1 seed; 150 generations are
 * precomputed as twice-box-blurred fields and scrubbed forward/back (ping-pong,
 * 0.09 gen/frame, smoothstep crossfade) so blobs merge then split forever.
 * Two-tone palette with inked borders, bilinearly upscaled. */
#include "../jellydazzle.h"
#include <math.h>

#define P44_GW   240
#define P44_GH   180
#define P44_N    (P44_GW * P44_GH)
#define P44_NGEN 70
#define P44_LO   12

static int8_t   p44_hist[P44_NGEN][P44_N];   /* smoothed fields, Q7 */
static int8_t   p44_cur[P44_N];
static int8_t   p44_nxt[P44_N];
static uint32_t p44_low[P44_N];
static int      p44_ready = 0;

static uint32_t p44_rs = 0x44BEEF17u;
static inline uint32_t p44_rnd(void) {
    p44_rs = p44_rs * 1664525u + 1013904223u;
    return p44_rs >> 16;
}

static void p44_box3(const float *src, float *dst, float *tmp) {
    for (int y = 0; y < P44_GH; y++) {
        const float *a = src + (long)((y == 0) ? P44_GH - 1 : y - 1) * P44_GW;
        const float *b = src + (long)y * P44_GW;
        const float *c = src + (long)((y == P44_GH - 1) ? 0 : y + 1) * P44_GW;
        float *o = tmp + (long)y * P44_GW;
        for (int x = 0; x < P44_GW; x++) o[x] = (a[x] + b[x] + c[x]) * (1.0f / 3.0f);
    }
    for (int y = 0; y < P44_GH; y++) {
        const float *r = tmp + (long)y * P44_GW;
        float *o = dst + (long)y * P44_GW;
        o[0] = (r[P44_GW - 1] + r[0] + r[1]) * (1.0f / 3.0f);
        for (int x = 1; x < P44_GW - 1; x++) o[x] = (r[x - 1] + r[x] + r[x + 1]) * (1.0f / 3.0f);
        o[P44_GW - 1] = (r[P44_GW - 2] + r[P44_GW - 1] + r[0]) * (1.0f / 3.0f);
    }
}

static void p44_build(void) {
    static int8_t u[P44_N], v[P44_N];
    static float fa[P44_N], fb[P44_N], ft[P44_N];
    for (int y = 0; y < P44_GH / 2; y++)
        for (int x = 0; x < P44_GW / 2; x++) {
            int8_t s = (p44_rnd() & 1) ? 1 : -1;
            u[y * P44_GW + x] = s;
            u[y * P44_GW + (P44_GW - 1 - x)] = s;
            u[(P44_GH - 1 - y) * P44_GW + x] = s;
            u[(P44_GH - 1 - y) * P44_GW + (P44_GW - 1 - x)] = s;
        }
    for (int g = 0; g < P44_NGEN; g++) {
        for (int y = 0; y < P44_GH; y++) {
            int ym = ((y == 0) ? P44_GH - 1 : y - 1) * P44_GW;
            int yc = y * P44_GW;
            int yp = ((y == P44_GH - 1) ? 0 : y + 1) * P44_GW;
            for (int x = 0; x < P44_GW; x++) {
                int xm = (x == 0) ? P44_GW - 1 : x - 1;
                int xp = (x == P44_GW - 1) ? 0 : x + 1;
                int n = u[ym + xm] + u[ym + x] + u[ym + xp]
                      + u[yc + xm] + u[yc + x] + u[yc + xp]
                      + u[yp + xm] + u[yp + x] + u[yp + xp];
                v[yc + x] = (int8_t)(n > 0 ? 1 : -1);
            }
        }
        for (int i = 0; i < P44_N; i++) { u[i] = v[i]; fa[i] = (float)v[i]; }
        p44_box3(fa, fb, ft);
        p44_box3(fb, fb, ft);
        for (int i = 0; i < P44_N; i++) {
            float q = fb[i] * 1.55f;                 /* firm up the blob edges */
            if (q > 1.0f) q = 1.0f; else if (q < -1.0f) q = -1.0f;
            p44_hist[g][i] = (int8_t)lrintf(q * 127.0f);
        }
    }
    p44_ready = 1;
}

static inline uint32_t p44_lerp(uint32_t a, uint32_t b, unsigned t) {
    unsigned s = 256u - t;
    uint32_t rb = (((a & 0xFF00FFu) * s + (b & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
    uint32_t g  = (((a & 0x00FF00u) * s + (b & 0x00FF00u) * t) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p44_blit(uint32_t *fb, int w, int h) {
    for (int y = 0; y < h; y++) {
        long fy = (((long)(2 * y + 1) * P44_GH) << 15) / h - (1L << 15);
        if (fy < 0) fy = 0;
        long fym = ((long)(P44_GH - 1)) << 16;
        if (fy > fym) fy = fym;
        int y0 = (int)(fy >> 16), y1 = y0 + 1 < P44_GH ? y0 + 1 : y0;
        unsigned wy = (unsigned)((fy >> 8) & 255);
        const uint32_t *r0 = p44_low + (long)y0 * P44_GW;
        const uint32_t *r1 = p44_low + (long)y1 * P44_GW;
        uint32_t *dst = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            long fx = (((long)(2 * x + 1) * P44_GW) << 15) / w - (1L << 15);
            if (fx < 0) fx = 0;
            long fxm = ((long)(P44_GW - 1)) << 16;
            if (fx > fxm) fx = fxm;
            int x0 = (int)(fx >> 16), x1 = x0 + 1 < P44_GW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((fx >> 8) & 255);
            uint32_t t = p44_lerp(r0[x0], r0[x1], wx);
            uint32_t b = p44_lerp(r1[x0], r1[x1], wx);
            dst[x] = 0xFF000000u | p44_lerp(t, b, wy);
        }
    }
}

void pattern_044(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!p44_ready) p44_build();
    float t = (float)frame;
    const int span = P44_NGEN - 1 - P44_LO;
    float m = fmodf(t * 0.09f, (float)(2 * span));
    float gp = (float)P44_LO + (m <= (float)span ? m : (float)(2 * span) - m);
    int i0 = (int)gp;
    if (i0 > P44_NGEN - 2) i0 = P44_NGEN - 2;
    float f = gp - (float)i0;
    f = f * f * (3.0f - 2.0f * f);
    int fw = (int)(f * 256.0f);
    if (fw > 256) fw = 256;
    int drift = (int)(t * 0.0005f * 32768.0f) + (int)(seed & 32767u);

    static uint8_t bri[256];
    static int16_t hue[256];
    for (int i = 0; i < 256; i++) {
        int val = i - 127;                       /* -127..128 ~ field*127 */
        int a = val < 0 ? -val : val;
        bri[i] = (uint8_t)(255.0f * (0.32f + 0.68f * ((float)a / 127.0f)));
        hue[i] = (int16_t)((val * 6200) / 127);
    }
    const int8_t *ha = p44_hist[i0], *hb = p44_hist[i0 + 1];
    for (int i = 0; i < P44_N; i++) {
        int val = (int)ha[i] + (((int)hb[i] - (int)ha[i]) * fw >> 8);
        int q = val + 127;
        if (q < 0) q = 0; else if (q > 255) q = 255;
        uint32_t c = pal[(drift + hue[q]) & JD_PAL_MASK];
        unsigned g = bri[q];
        p44_low[i] = ((((((c >> 16) & 255) * g) >> 8) << 16)
                    | (((((c >> 8) & 255) * g) >> 8) << 8)
                    |  (((c & 255) * g) >> 8));
    }
    p44_blit(fb, w, h);
    (void)p44_cur; (void)p44_nxt;
}
