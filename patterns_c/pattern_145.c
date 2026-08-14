/* 145 Aurora Curtains — six hanging sheets of airglow.
 * Each curtain is a wavy top edge  yTop(x) = A + a1 sin(k1 x + w1 t)
 * + a2 sin(k2 x - w2 t), from which light hangs downward with an exponential
 * falloff; the vertical ray structure is a third, higher-frequency sine in x
 * that scrolls at its own rate, so the rays visibly slide along the curtain
 * the way real auroral rays do. Depth below the edge drives the palette walk,
 * giving the classic green crown / violet fringe (or whatever two-thirds of
 * the current scheme happens to be). Static star field, no twinkle — nothing
 * in this pattern changes faster than a few tenths of a pixel per frame.
 * 480x360 float canvas, bilinear upscale. Black sky = free layering. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define P145_GW 480
#define P145_GH 360
#define P145_N  (P145_GW * P145_GH)
#define P145_NC 6
#define P145_NSTAR 220

static float p145_can[P145_N * 3];
static uint8_t p145_img[P145_N * 3];
static float p145_sin[2048];
static uint8_t p145_fall[512];         /* exp(-u) hang-down profile */
static int p145_starp[P145_NSTAR];
static uint8_t p145_starv[P145_NSTAR];
static uint8_t p145_sky[P145_GH];
static int p145_ready;
static int p145_uw = -1;
static int *p145_xi;
static uint8_t *p145_fx;

static inline float p145_s(float a)
{
    return p145_sin[((int)(a * 325.9493f + 32768.5f)) & 2047];
}

static void p145_init(void)
{
    int i;
    uint32_t s = 0xB16B00B5u;
    for (i = 0; i < 2048; i++)
        p145_sin[i] = sinf((float)i * (6.28318531f / 2048.0f));
    for (i = 0; i < 512; i++)
        p145_fall[i] = (uint8_t)(255.0f * expf(-(float)i * (1.0f / 128.0f)));
    for (i = 0; i < P145_NSTAR; i++) {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        p145_starp[i] = (int)(s % (uint32_t)P145_N);
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        p145_starv[i] = (uint8_t)(40 + (s & 127));
    }
    for (i = 0; i < P145_GH; i++) {
        float u = (float)i / (float)P145_GH;
        p145_sky[i] = (uint8_t)(26.0f * (1.0f - u) + 4.0f);
    }
    p145_ready = 1;
}

static void p145_upscale(uint32_t *fb, int w, int h)
{
    if (w != p145_uw) {
        free(p145_xi); free(p145_fx);
        p145_xi = (int *)malloc(sizeof(int) * (size_t)w);
        p145_fx = (uint8_t *)malloc((size_t)w);
        for (int x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (P145_GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8;
            if (xi > P145_GW - 2) { xi = P145_GW - 2; q = (P145_GW - 1) * 256; }
            p145_xi[x] = xi * 3; p145_fx[x] = (uint8_t)(q & 255);
        }
        p145_uw = w;
    }
    for (int y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (P145_GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8;
        if (yi > P145_GH - 2) { yi = P145_GH - 2; qy = (P145_GH - 1) * 256; }
        int fy = qy & 255;
        const uint8_t *r0 = p145_img + (size_t)yi * P145_GW * 3;
        const uint8_t *r1 = r0 + P145_GW * 3;
        uint32_t *out = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) {
            int X = p145_xi[x], fx = p145_fx[x], c[3];
            for (int k = 0; k < 3; k++) {
                int t0 = r0[X + k] + (((r0[X + 3 + k] - r0[X + k]) * fx) >> 8);
                int t1 = r1[X + k] + (((r1[X + 3 + k] - r1[X + k]) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16)
                   | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

void pattern_145(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (!p145_ready) p145_init();
    const float t = (float)frame;
    const float sd = (float)(seed & 255u) * 0.0245437f;

    memset(p145_can, 0, sizeof p145_can);

    static const float cbase[P145_NC] = { 34.0f, 78.0f, 124.0f, 58.0f, 148.0f, 186.0f };
    static const float clen [P145_NC] = { 245.0f, 190.0f, 150.0f, 210.0f, 158.0f, 128.0f };
    static const float cint [P145_NC] = { 1.00f, 0.82f, 0.62f, 0.74f, 0.55f, 0.45f };

    for (int c = 0; c < P145_NC; c++) {
        float fc = (float)c;
        float k1 = 0.0135f + 0.0042f * fc, k2 = 0.0301f - 0.0033f * fc;
        float k3 = 0.0930f + 0.0175f * fc;
        float w1 = 0.0031f + 0.0007f * fc, w2 = -0.0022f - 0.0005f * fc;
        float w3 = 0.0016f * (c & 1 ? -1.0f : 1.0f);
        float a1 = 26.0f + 7.0f * fc, a2 = 13.0f - 1.1f * fc;
        float base = cbase[c] + 16.0f * p145_s(t * 0.00042f + fc * 1.7f + sd);
        float len = clen[c];
        float inten = cint[c] * (0.72f + 0.28f * p145_s(t * 0.00061f + fc * 2.3f + sd));
        int ci = (int)(t * 0.9f) + (int)(seed & 8191u) + c * 260;
        float invlen = 128.0f / len;

        for (int x = 0; x < P145_GW; x++) {
            float fx = (float)x;
            float top = base + a1 * p145_s(fx * k1 + t * w1 + sd + fc)
                             + a2 * p145_s(fx * k2 + t * w2 + fc * 2.1f);
            float ray = 0.42f + 0.58f * p145_s(fx * k3 + t * w3 + fc);
            ray *= ray;                                   /* sharpen the rays */
            float amp = inten * ray;
            if (amp < 0.012f) continue;
            int y0 = (int)top;
            if (y0 < 0) y0 = 0;
            int y1 = (int)(top + len);
            if (y1 > P145_GH) y1 = P145_GH;
            float fade = 1.0f;
            for (int y = y0; y < y1; y++) {
                float u = (float)y - top;
                if (u < 0.0f) continue;
                int fi = (int)(u * invlen);
                if (fi > 511) break;
                float v = amp * (float)p145_fall[fi] * (1.0f / 255.0f);
                /* soft leading edge so the crown is not a hard line */
                if (u < 6.0f) v *= (u + 1.0f) * 0.143f;
                uint32_t col = pal[(uint32_t)(ci + (int)(u * 14.0f)) & JD_PAL_MASK];
                float *p = p145_can + ((size_t)y * P145_GW + x) * 3;
                p[0] += v * (float)((col >> 16) & 255u);
                p[1] += v * (float)((col >> 8) & 255u);
                p[2] += v * (float)(col & 255u);
            }
            (void)fade;
        }
    }

    /* stars + sky, then tone-limit */
    for (int i = 0; i < P145_NSTAR; i++) {
        float *p = p145_can + (size_t)p145_starp[i] * 3;
        float v = (float)p145_starv[i];
        p[0] += v; p[1] += v; p[2] += v * 1.15f;
    }
    uint32_t sky = pal[(uint32_t)((int)(t * 0.6f) + 2200) & JD_PAL_MASK];
    int skr = (int)((sky >> 16) & 255u), skg = (int)((sky >> 8) & 255u);
    int skb = (int)(sky & 255u);
    uint8_t *o = p145_img;
    const float *ca = p145_can;
    for (int y = 0; y < P145_GH; y++) {
        int sv = p145_sky[y];
        int ar = (skr * sv) >> 9, ag = (skg * sv) >> 9, ab = (skb * sv) >> 8;
        for (int x = 0; x < P145_GW; x++) {
            int r = ar + (int)ca[0], g = ag + (int)ca[1], b = ab + (int)ca[2];
            ca += 3;
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            *o++ = (uint8_t)r; *o++ = (uint8_t)g; *o++ = (uint8_t)b;
        }
    }
    p145_upscale(fb, w, h);
}
