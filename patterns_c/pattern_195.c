/* 195 Aperture Star — Fraunhofer diffraction through a turning polygon.
 * Far-field diffraction from an n-sided aperture throws one ray perpendicular
 * to every edge, and along each ray the amplitude rings as a sinc, which is
 * exactly why every camera photograph of a bright light wears a fringed star.
 * The frame builds that model directly: a Gaussian across each ray times a
 * sinc-squared along it, over an Airy-like ring core. Three sample wavelengths
 * are traced separately and the long-wavelength copy reaches further, so each
 * ray fans into its own little spectrum. The aperture turns slowly, the rings
 * pulse; everything outside the star stays black, which makes this a glow to
 * lay over other things rather than a picture of its own. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p195_up;

#define P195_W 320
#define P195_H 240
#define P195_TAU 6.28318530717958647692f

static float p195_acc[P195_W * P195_H * 3];
static unsigned char p195_img[P195_W * P195_H * 3];
static unsigned char p195_tone[1024];
static int *p195_xm;
static int p195_xmw;
static int p195_tone_ok;
static uint32_t p195_rs = 1u;

static float p195_rf(void)
{
    p195_rs ^= p195_rs << 13; p195_rs ^= p195_rs >> 17; p195_rs ^= p195_rs << 5;
    return (float)(p195_rs >> 8) * (1.0f / 16777216.0f);
}

static void p195_tone_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (6.50f / 1024.0f)));
        p195_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
    }
    p195_tone_ok = 1;
}

/* palette sample, brightness-normalised so dark ramp zones still read as light */
static void p195_col(const uint32_t *pal, float hue, float lift, float *out)
{
    uint32_t p; float r, g, b, mx;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
    mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    out[0] = lift + (1.0f - lift) * r / mx;
    out[1] = lift + (1.0f - lift) * g / mx;
    out[2] = lift + (1.0f - lift) * b / mx;
}


static void p195_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P195_W * P195_H * 3; i++) {
        int ti = (int)(p195_acc[i] * 256.0f);
        p195_img[i] = p195_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p195_xmw != w) {
        free(p195_xm);
        p195_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p195_xm[x] = (int)(((long long)x * (P195_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p195_xmw = w;
    }
    jd_up_blit(&p195_up, fb, w, h, p195_img, P195_W, P195_H);
}

#define P195_NL 3

static uint32_t p195_seedc = 0xFFFFFFFFu;
static float p195_h0, p195_hw, p195_rk, p195_spin;
static int p195_nn;
static const float p195_il[P195_NL] = {1.190476f, 1.0f, 0.833333f};
static float p195_tint[P195_NL][3];

static void p195_build(uint32_t seed)
{
    p195_rs = seed ? seed * 2654435761u + 0x165667B1u : 0x195u;
    p195_rf(); p195_rf();
    p195_h0 = p195_rf();
    p195_hw = 0.06f + p195_rf() * 0.52f;
    p195_nn = 3 + (int)(p195_rf() * 6.0f);          /* 3..8 aperture sides */
    p195_rk = 3.4f + p195_rf() * 2.6f;
    p195_spin = (p195_rf() < 0.5f ? -1.0f : 1.0f) * (0.00042f + p195_rf() * 0.00050f);
    p195_seedc = seed;
    if (!p195_tone_ok) p195_tone_init();
}

void pattern_195(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, a0, inv, cx, cy, rph, puls;
    float dcx[10], dsx[10];
    int x, y, k, L;
    (void)sl;
    if (p195_seedc != seed) p195_build(seed);
    for (L = 0; L < P195_NL; L++)
        p195_col(pal, p195_h0 + p195_hw * ((float)L / (float)(P195_NL - 1)),
                 0.05f, p195_tint[L]);

    a0 = t * p195_spin;
    for (k = 0; k < p195_nn; k++) {
        float a = a0 + (float)k * (3.14159265f / (float)p195_nn);
        dcx[k] = cosf(a); dsx[k] = sinf(a);
    }
    inv = 12.0f / (float)P195_H;
    cx = P195_W * 0.5f; cy = P195_H * 0.5f;
    rph = t * 0.0085f;
    puls = 0.80f + 0.20f * sinf(t * 0.00058f);

    for (y = 0; y < P195_H; y++) {
        float v = ((float)y - cy) * inv;
        float *dst = p195_acc + y * P195_W * 3;
        for (x = 0; x < P195_W; x++) {
            float u = ((float)x - cx) * inv;
            float r = sqrtf(u * u + v * v);
            float o0 = 0.0f, o1 = 0.0f, o2 = 0.0f;
            float e = expf(-r * 0.62f);
            float mask = 1.0f - e * e * e;
            float lac[P195_NL];
            for (L = 0; L < P195_NL; L++) {
                float ph = r * p195_rk * p195_il[L] - rph;
                float cc = 0.5f + 0.5f * cosf(ph);
                lac[L] = 0.36f * e * cc * cc;
            }
            for (k = 0; k < p195_nn; k++) {
                float p = u * dcx[k] + v * dsx[k];
                float q = -u * dsx[k] + v * dcx[k];
                if (q > 0.317f || q < -0.317f) continue;   /* misses every ray */
                if (p < 0.0f) p = -p;
                for (L = 0; L < P195_NL; L++) {
                    float il = p195_il[L];
                    float qq = q * (8.5f * il), sp, ss;
                    if (qq > 3.2f || qq < -3.2f) continue;
                    sp = p * (5.5f * il);
                    ss = (0.5f + 0.5f * cosf(2.0f * sp)) / (1.0f + 0.42f * sp);
                    lac[L] += 1.25f * mask * ss * expf(-qq * qq * 0.5f);
                }
            }
            for (L = 0; L < P195_NL; L++) {
                float acc = lac[L] * puls * 0.72f;
                o0 += acc * p195_tint[L][0];
                o1 += acc * p195_tint[L][1];
                o2 += acc * p195_tint[L][2];
            }
            dst[x * 3 + 0] = o0; dst[x * 3 + 1] = o1; dst[x * 3 + 2] = o2;
        }
    }
    p195_blit(fb, w, h);
}
