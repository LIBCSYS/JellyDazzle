/* 142 Attractor Smoke — a Peter de Jong strange attractor as drifting smoke.
 *   x' = sin(a y) - cos(b x)      y' = sin(c x) - cos(d y)
 * 6000 walkers take 30 steps each per frame and deposit into a 512x384
 * density field that decays 8% a frame, so the picture is a *time-averaged*
 * exposure of the attractor rather than a per-frame shot — no shot noise, no
 * flicker. The four coefficients are on very slow independent sinusoids, so
 * the filament bundle continuously reshapes: whorls open, fold, and close.
 * Tone-mapped 1-exp(-kd), bilinear upscale. Mostly black, so it screens or
 * maxes cleanly over a ground layer. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define P142_GW 512
#define P142_GH 384
#define P142_NP 6000
#define P142_STEP 30

static float p142_acc[P142_GH * P142_GW];
static uint8_t p142_img[P142_GH * P142_GW * 3];
static float p142_px[P142_NP], p142_py[P142_NP];
static float p142_sin[4096];
static uint8_t p142_tone[2048];
static uint32_t p142_rs = 0x2545F491u;
static int p142_ready, p142_last = -1;
static int p142_uw = -1, p142_uh = -1;
static int *p142_xi;
static uint8_t *p142_fx;

static uint32_t p142_rnd(void)
{
    p142_rs ^= p142_rs << 13; p142_rs ^= p142_rs >> 17; p142_rs ^= p142_rs << 5;
    return p142_rs;
}
static float p142_r01(void) { return (float)(p142_rnd() >> 8) * (1.0f / 16777216.0f); }

static inline float p142_s(float a)
{
    return p142_sin[((int)(a * 651.8986f + 65536.5f)) & 4095];
}
static inline float p142_c(float a)
{
    return p142_sin[((int)(a * 651.8986f + 65536.5f) + 1024) & 4095];
}

static void p142_init(void)
{
    int i;
    for (i = 0; i < 4096; i++)
        p142_sin[i] = sinf((float)i * (6.28318531f / 4096.0f));
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (1.0f / 90.0f)));
        p142_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    p142_ready = 1;
}

static void p142_reseed(uint32_t seed)
{
    p142_rs = seed ? (seed * 2654435761u) | 1u : 0x2545F491u;
    for (int i = 0; i < P142_NP; i++) {
        p142_px[i] = p142_r01() * 4.0f - 2.0f;
        p142_py[i] = p142_r01() * 4.0f - 2.0f;
    }
    memset(p142_acc, 0, sizeof p142_acc);
}

static void p142_upscale(uint32_t *fb, int w, int h)
{
    if (w != p142_uw) {
        free(p142_xi); free(p142_fx);
        p142_xi = (int *)malloc(sizeof(int) * (size_t)w);
        p142_fx = (uint8_t *)malloc((size_t)w);
        for (int x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (P142_GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8;
            if (xi > P142_GW - 2) { xi = P142_GW - 2; q = (P142_GW - 1) * 256; }
            p142_xi[x] = xi * 3; p142_fx[x] = (uint8_t)(q & 255);
        }
        p142_uw = w;
    }
    p142_uh = h;
    for (int y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (P142_GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8;
        if (yi > P142_GH - 2) { yi = P142_GH - 2; qy = (P142_GH - 1) * 256; }
        int fy = qy & 255;
        const uint8_t *r0 = p142_img + (size_t)yi * P142_GW * 3;
        const uint8_t *r1 = r0 + P142_GW * 3;
        uint32_t *out = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) {
            int X = p142_xi[x], fx = p142_fx[x], c[3];
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

void pattern_142(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    if (!p142_ready) p142_init();
    if (p142_last < 0 || (sl == 0 && p142_last != 0)) p142_reseed(seed);
    p142_last = sl;

    const float t = (float)frame;
    const float ph = (float)(seed & 255u) * 0.0245437f;
    const float a = 1.641f + 0.30f * sinf(t * 0.00061f + ph);
    const float b = 1.902f + 0.28f * sinf(t * 0.00047f + ph * 1.3f + 1.3f);
    const float c = 0.316f + 0.35f * sinf(t * 0.00039f + ph * 0.7f + 2.7f);
    const float d = 1.525f + 0.28f * sinf(t * 0.00053f + ph * 1.9f + 0.8f);

    /* decay the exposure */
    for (int i = 0; i < P142_GH * P142_GW; i++) p142_acc[i] *= 0.95f;

    /* a few walkers restart each frame so no basin can go permanently empty */
    for (int k = 0; k < 24; k++) {
        int i = (int)(p142_rnd() % P142_NP);
        p142_px[i] = p142_r01() * 4.0f - 2.0f;
        p142_py[i] = p142_r01() * 4.0f - 2.0f;
    }

    const float vr = t * 0.00072f + ph;          /* slow view rotation */
    const float vc = cosf(vr), vs = sinf(vr);
    const float sc = (float)P142_GH * 0.255f;
    const float ox = (float)P142_GW * 0.5f, oy = (float)P142_GH * 0.5f;
    const float sx = sc * vc, sxy = -sc * vs;
    const float syx = sc * vs, sy = sc * vc;

    for (int i = 0; i < P142_NP; i++) {
        float x = p142_px[i], y = p142_py[i];
        for (int s = 0; s < P142_STEP; s++) {
            float nx = p142_s(a * y) - p142_c(b * x);
            float ny = p142_s(c * x) - p142_c(d * y);
            x = nx; y = ny;
            float fxp = x * sx + y * sxy + ox, fyp = x * syx + y * sy + oy;
            int xi = (int)fxp, yi = (int)fyp;
            if ((unsigned)xi < P142_GW - 1u && (unsigned)yi < P142_GH - 1u) {
                float u = fxp - (float)xi, v = fyp - (float)yi;
                float *p = p142_acc + (size_t)yi * P142_GW + xi;
                p[0] += (1.0f - u) * (1.0f - v);
                p[1] += u * (1.0f - v);
                p[P142_GW] += (1.0f - u) * v;
                p[P142_GW + 1] += u * v;
            }
        }
        p142_px[i] = x; p142_py[i] = y;
    }

    const int cidx = (int)(t * 1.3f) + (int)(seed & 8191u);
    uint8_t *o = p142_img;
    for (int y = 0; y < P142_GH; y++) {
        const float *ar = p142_acc + (size_t)y * P142_GW;
        for (int x = 0; x < P142_GW; x++) {
            int ti = (int)(ar[x] * 2.6f);
            if (ti > 2047) ti = 2047;
            int v8 = p142_tone[ti];
            uint32_t col = pal[(uint32_t)(cidx + ((v8 * 5600) >> 8)
                              + ((x + y) * 3)) & JD_PAL_MASK];
            *o++ = (uint8_t)((((col >> 16) & 255u) * (uint32_t)v8) >> 8);
            *o++ = (uint8_t)((((col >> 8) & 255u) * (uint32_t)v8) >> 8);
            *o++ = (uint8_t)(((col & 255u) * (uint32_t)v8) >> 8);
        }
    }
    p142_upscale(fb, w, h);
}
