/* 199 Talbot Carpet — the self-image lattice behind a diffraction grating.
 * Light leaving a periodic grating reconstructs a perfect copy of the grating
 * at the Talbot distance, a negated copy at half of it, and rational fractions
 * of the period everywhere in between — the field in the plane behind the
 * grating is a genuine fractal, the Talbot carpet. It is computed here exactly
 * as the physics writes it: the grating's Fourier coefficients c_n, each turned
 * by the Fresnel phase -pi*lambda*z*n^2, summed and squared. Screen x is the
 * grating coordinate, screen y is distance from the grating, so the whole
 * carpet is on show at once; it drifts slowly downstream while the grating's
 * duty cycle breathes, which reshapes every branch of the lattice together. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p199_up;

#define P199_W 480
#define P199_H 360
#define P199_TAU 6.28318530717958647692f

static float p199_acc[P199_W * P199_H * 3];
static unsigned char p199_img[P199_W * P199_H * 3];
static unsigned char p199_tone[1024];
static int *p199_xm;
static int p199_xmw;
static int p199_tone_ok;
static uint32_t p199_rs = 1u;

static float p199_rf(void)
{
    p199_rs ^= p199_rs << 13; p199_rs ^= p199_rs >> 17; p199_rs ^= p199_rs << 5;
    return (float)(p199_rs >> 8) * (1.0f / 16777216.0f);
}

static void p199_tone_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (6.00f / 1024.0f)));
        p199_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
    }
    p199_tone_ok = 1;
}

/* palette sample, brightness-normalised so dark ramp zones still read as light */
static void p199_col(const uint32_t *pal, float hue, float lift, float *out)
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


static void p199_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P199_W * P199_H * 3; i++) {
        int ti = (int)(p199_acc[i] * 256.0f);
        p199_img[i] = p199_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p199_xmw != w) {
        free(p199_xm);
        p199_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p199_xm[x] = (int)(((long long)x * (P199_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p199_xmw = w;
    }
    jd_up_blit(&p199_up, fb, w, h, p199_img, P199_W, P199_H);
}

#define P199_M 9

static uint32_t p199_seedc = 0xFFFFFFFFu;
static float p199_h0, p199_hw, p199_zsp, p199_xsp, p199_per;
static float p199_cn[P199_M + 1];
static float p199_ctb[P199_M + 1][P199_W];
static float p199_ctab[257][3];

static void p199_build(uint32_t seed)
{
    p199_rs = seed ? seed * 2654435761u + 0xCC9E2D51u : 0x199u;
    p199_rf(); p199_rf();
    p199_h0  = p199_rf();
    p199_hw  = 0.07f + p199_rf() * 0.55f;
    p199_zsp = (p199_rf() < 0.5f ? -1.0f : 1.0f) * (0.00028f + p199_rf() * 0.00030f);
    p199_xsp = (p199_rf() < 0.5f ? -1.0f : 1.0f) * (0.00018f + p199_rf() * 0.00026f);
    p199_per = 1.55f + p199_rf() * 1.45f;             /* periods across the frame */
    p199_seedc = seed;
    if (!p199_tone_ok) p199_tone_init();
}

void pattern_199(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, duty, x0, z0, dz, gain;
    int x, y, n, i;
    (void)sl;
    if (p199_seedc != seed) p199_build(seed);
    for (i = 0; i <= 256; i++) {
        float v = (float)i * (1.0f / 256.0f);
        float hv = v * v * (3.0f - 2.0f * v);
        p199_col(pal, p199_h0 + p199_hw * hv, 0.06f, p199_ctab[i]);
        p199_ctab[i][0] *= v; p199_ctab[i][1] *= v; p199_ctab[i][2] *= v;
    }

    duty = 0.36f + 0.16f * sinf(t * 0.00037f);
    p199_cn[0] = duty;
    for (n = 1; n <= P199_M; n++)
        p199_cn[n] = sinf(3.14159265f * (float)n * duty) / (3.14159265f * (float)n);
    x0 = t * p199_xsp;
    z0 = 0.35f + t * p199_zsp;
    dz = 1.12f / (float)P199_H;
    for (n = 1; n <= P199_M; n++) {
        float f = P199_TAU * (float)n * p199_per / (float)P199_W;
        float b = P199_TAU * (float)n * x0;
        for (x = 0; x < P199_W; x++) p199_ctb[n][x] = 2.0f * p199_cn[n] * cosf((float)x * f + b);
    }
    gain = 76.0f / (duty + 0.02f);

    for (y = 0; y < P199_H; y++) {
        float z = z0 + (float)y * dz;
        float re[P199_M + 1], im[P199_M + 1];
        float *dst = p199_acc + y * P199_W * 3;
        for (n = 1; n <= P199_M; n++) {
            float th = -3.14159265f * z * (float)(n * n);
            re[n] = cosf(th); im[n] = sinf(th);
        }
        for (x = 0; x < P199_W; x++) {
            float sr = p199_cn[0], si = 0.0f, I;
            const float *cp;
            int idx;
            for (n = 1; n <= P199_M; n++) {
                float c = p199_ctb[n][x];
                sr += c * re[n]; si += c * im[n];
            }
            I = (sr * sr + si * si) * gain;
            idx = (int)I; if (idx > 256) idx = 256; if (idx < 0) idx = 0;
            cp = p199_ctab[idx];
            dst[x * 3 + 0] = cp[0]; dst[x * 3 + 1] = cp[1]; dst[x * 3 + 2] = cp[2];
        }
    }
    p199_blit(fb, w, h);
}
