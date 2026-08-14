/* 162 Orbital Lobes — hydrogen-like probability clouds, drawn as light.
 * The field is |psi|^2 for a separable orbital: a radial part r^l e^-r/n with
 * a drifting number of radial nodes, times an angular part that crossfades
 * between m and m+1 lobes so the petal count changes without a seam. Two
 * orbitals are superposed at different spins, and positive and negative phase
 * are tinted apart, giving the two-tone lobes of a textbook wavefunction.
 * Everything outside the exponential tail is black: a sparse overlay. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p162_up;

#define P162_LW 480
#define P162_LH 360
#define P162_TAU 6.28318530717958647692f

static uint8_t p162_img[P162_LW * P162_LH * 3];
static int    *p162_xm;
static int     p162_xm_w;
static uint8_t p162_ramp[256][3];

/* 256-entry hue ramp lifted from the live palette. Palette schemes contain
 * near-black stretches; a too-dark sample inherits its predecessor so the
 * figure never gets punched full of dark holes. */
static void p162_build_ramp(const uint32_t *pal, int base, int span)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * span) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p162_ramp[i][0] = p162_ramp[i-1][0];
                     p162_ramp[i][1] = p162_ramp[i-1][1];
                     p162_ramp[i][2] = p162_ramp[i-1][2]; }
            else   { p162_ramp[i][0] = p162_ramp[i][1] = p162_ramp[i][2] = 255; }
            continue;
        }
        p162_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p162_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p162_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

/* bilinear upscale of the low-res image onto the real framebuffer */
static void p162_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p162_xm_w != w) {
        free(p162_xm);
        p162_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p162_xm[x] = (int)(((long long)x * (P162_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p162_xm_w = w;
    }
    jd_up_blit(&p162_up, fb, w, h, p162_img, P162_LW, P162_LH);
}


#define P162_RN 1024                     /* radial LUT resolution   */
#define P162_AN 1024                     /* angular LUT resolution  */

static uint16_t p162_rfix[P162_LW * P162_LH];
static uint16_t p162_afix[P162_LW * P162_LH];
static float    p162_R1[P162_RN], p162_R2[P162_RN];
static float    p162_A1[P162_AN], p162_A2[P162_AN];
static uint8_t  p162_glow[1024];
static int      p162_ready;

static void p162_build_geom(void)
{
    int x, y;
    float inv = 1.0f / (0.46f * (float)P162_LH);
    for (y = 0; y < P162_LH; y++) {
        float dy = (float)y - (float)P162_LH * 0.5f + 0.5f;
        for (x = 0; x < P162_LW; x++) {
            float dx = (float)x - (float)P162_LW * 0.5f + 0.5f;
            float rr = sqrtf(dx * dx + dy * dy) * inv;
            float aa = atan2f(dy, dx) * (1.0f / P162_TAU);
            int i = y * P162_LW + x, ai;
            rr *= 1024.0f;
            p162_rfix[i] = rr > 65000.0f ? 65000 : (uint16_t)rr;
            aa -= floorf(aa);
            ai = (int)(aa * (float)P162_AN);
            p162_afix[i] = (uint16_t)(ai & (P162_AN - 1));
        }
    }
    for (x = 0; x < 1024; x++) {
        float u = (float)x * (1.0f / 1023.0f);        /* normalised |psi|^2 */
        float v = powf(u, 0.50f);                     /* lift the tails     */
        p162_glow[x] = (uint8_t)(v * 255.0f);
    }
    p162_ready = 1;
}

/* angular part: cos(m.theta) crossfaded between two integer lobe counts, so a
 * changing petal count is an amplitude blend and never a discontinuity */
static void p162_ang(float *dst, float m, float spin)
{
    int i, m0 = (int)m;
    float f = m - (float)m0, g = 1.0f - f;
    for (i = 0; i < P162_AN; i++) {
        float th = (float)i * (P162_TAU / (float)P162_AN) + spin;
        dst[i] = g * cosf((float)m0 * th) + f * cosf((float)(m0 + 1) * th);
    }
}

static float p162_rad(float *dst, float l, float n, float nodes, float ph)
{
    int i;
    float mx = 1e-6f;
    for (i = 0; i < P162_RN; i++) {
        float r = (float)i * (1.0f / (float)P162_RN);
        float v = powf(r, l) * expf(-r * n) * sinf(nodes * r * 3.14159265f + ph);
        dst[i] = v;
        if (fabsf(v) > mx) mx = fabsf(v);
    }
    mx = 1.0f / mx;
    for (i = 0; i < P162_RN; i++) dst[i] *= mx;
    return mx;
}

void pattern_162(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float s = (float)(seed & 1023) * 0.00613f;
    float m1, m2, w2, zoom;
    int i, n, zq, hb;
    (void)sl;

    if (!p162_ready) p162_build_geom();
    p162_build_ramp(pal, (int)(t * 1.7f) + (int)(seed & 32767), 112);

    m1 = 2.0f + 3.0f * (0.5f + 0.5f * sinf(t * 0.00029f + s));
    m2 = 1.0f + 4.0f * (0.5f + 0.5f * sinf(t * 0.00021f + s * 1.9f + 2.3f));
    w2 = 0.35f + 0.30f * sinf(t * 0.00043f + s * 0.7f);
    zoom = 1.0f + 0.13f * sinf(t * 0.00033f + 1.7f);

    p162_ang(p162_A1, m1, t * 0.0021f);
    p162_ang(p162_A2, m2, -t * 0.0014f + 1.3f);
    p162_rad(p162_R1, 1.1f + 0.9f * (0.5f + 0.5f * sinf(t * 0.00025f + s)),
             4.6f, 2.0f + 1.6f * (0.5f + 0.5f * sinf(t * 0.00019f + 0.9f)),
             0.35f + 0.25f * sinf(t * 0.00027f));
    p162_rad(p162_R2, 0.7f + 1.3f * (0.5f + 0.5f * sinf(t * 0.00017f + 2.2f)),
             5.4f, 3.0f + 2.2f * (0.5f + 0.5f * sinf(t * 0.00023f + 3.4f)),
             1.10f + 0.30f * sinf(t * 0.00031f + 1.1f));

    zq = (int)(1024.0f / zoom);
    hb = (int)(t * 0.05f);
    n  = P162_LW * P162_LH;
    for (i = 0; i < n; i++) {
        int ri = ((int)p162_rfix[i] * zq) >> 10;
        int ai, lum, hue, o = i * 3;
        float v, e;
        const uint8_t *cp;
        if (ri > P162_RN - 1) { p162_img[o] = p162_img[o+1] = p162_img[o+2] = 0; continue; }
        ai = p162_afix[i];
        v  = p162_R1[ri] * p162_A1[ai] + w2 * p162_R2[ri] * p162_A2[ai];
        e  = v * v * 1.05f;
        if (e < 0.0035f) { p162_img[o] = p162_img[o+1] = p162_img[o+2] = 0; continue; }
        if (e > 1.0f) e = 1.0f;
        lum = p162_glow[(int)(e * 1023.0f)];
        hue = (hb + (ri >> 5) + (v > 0.0f ? 0 : 88)) & 255;
        cp  = p162_ramp[hue];
        p162_img[o + 0] = (uint8_t)((cp[0] * lum) >> 8);
        p162_img[o + 1] = (uint8_t)((cp[1] * lum) >> 8);
        p162_img[o + 2] = (uint8_t)((cp[2] * lum) >> 8);
    }
    p162_blit(fb, w, h);
}
