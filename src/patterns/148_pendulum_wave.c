/* 148 Pendulum Wave — the physics-lecture kinetic sculpture, mirrored.
 * Thirty pendulums hang from a rail, the i-th tuned to make exactly (K+i)
 * swings in one 120-second cycle (K = 18, so 18..47 swings), and its drawn
 * length runs as 1/sqrt(K+i) — the true 1/w^2 ladder collapses the short end
 * into the rail, so the visual ladder is stretched while the frequencies stay
 * exact. Started in phase they dephase
 * into a travelling wave, then braid into two counter-running waves, then
 * four, then scatter into apparent chaos, then re-converge — the whole cycle
 * is exactly periodic and takes two minutes. A second rail below runs the
 * mirror image, so the piece closes as a chandelier. Bobs and wires are
 * additively painted into a 640x480 canvas that decays 4.5% a frame, which
 * leaves long comet ribbons showing where each bob has been. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define P148_GW 640
#define P148_GH 480
#define P148_N  (P148_GW * P148_GH)
#define P148_NP 30
#define P148_KER 9

static float p148_can[P148_N * 3];
static uint8_t p148_img[P148_N * 3];
static float p148_ker[(2 * P148_KER + 1) * (2 * P148_KER + 1)];
static uint8_t p148_tone[1024];
static int p148_ready, p148_last = -1;
static int p148_uw = -1;
static int *p148_xi;
static uint8_t *p148_fx;

static void p148_init(void)
{
    int dx, dy, i = 0;
    for (dy = -P148_KER; dy <= P148_KER; dy++)
        for (dx = -P148_KER; dx <= P148_KER; dx++) {
            float d2 = (float)(dx * dx + dy * dy);
            p148_ker[i++] = expf(-d2 * (1.0f / 15.0f));
        }
    for (i = 0; i < 1024; i++) {
        float a = (float)i;
        p148_tone[i] = (uint8_t)(255.0f * a / (a + 300.0f));
    }
    p148_ready = 1;
}

static inline void p148_dot(float fx, float fy, float r, float g, float b, float amp)
{
    int cx = (int)fx, cy = (int)fy;
    if (cx < P148_KER || cy < P148_KER
        || cx >= P148_GW - P148_KER || cy >= P148_GH - P148_KER) return;
    const float *k = p148_ker;
    for (int dy = -P148_KER; dy <= P148_KER; dy++) {
        float *p = p148_can + ((size_t)(cy + dy) * P148_GW + cx - P148_KER) * 3;
        for (int dx = -P148_KER; dx <= P148_KER; dx++) {
            float v = *k++ * amp;
            p[0] += r * v; p[1] += g * v; p[2] += b * v;
            p += 3;
        }
    }
}

static inline void p148_pt(float fx, float fy, float r, float g, float b, float amp)
{
    int xi = (int)fx, yi = (int)fy;
    if ((unsigned)xi >= (unsigned)(P148_GW - 1) || (unsigned)yi >= (unsigned)(P148_GH - 1))
        return;
    float u = fx - (float)xi, v = fy - (float)yi;
    float *p = p148_can + ((size_t)yi * P148_GW + xi) * 3;
    float w0 = (1.0f - u) * (1.0f - v) * amp, w1 = u * (1.0f - v) * amp;
    float w2 = (1.0f - u) * v * amp, w3 = u * v * amp;
    p[0] += r * w0; p[1] += g * w0; p[2] += b * w0;
    p[3] += r * w1; p[4] += g * w1; p[5] += b * w1;
    p += P148_GW * 3;
    p[0] += r * w2; p[1] += g * w2; p[2] += b * w2;
    p[3] += r * w3; p[4] += g * w3; p[5] += b * w3;
}

static void p148_upscale(uint32_t *fb, int w, int h)
{
    if (w != p148_uw) {
        free(p148_xi); free(p148_fx);
        p148_xi = (int *)malloc(sizeof(int) * (size_t)w);
        p148_fx = (uint8_t *)malloc((size_t)w);
        for (int x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (P148_GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8;
            if (xi > P148_GW - 2) { xi = P148_GW - 2; q = (P148_GW - 1) * 256; }
            p148_xi[x] = xi * 3; p148_fx[x] = (uint8_t)(q & 255);
        }
        p148_uw = w;
    }
    for (int y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (P148_GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8;
        if (yi > P148_GH - 2) { yi = P148_GH - 2; qy = (P148_GH - 1) * 256; }
        int fy = qy & 255;
        const uint8_t *r0 = p148_img + (size_t)yi * P148_GW * 3;
        const uint8_t *r1 = r0 + P148_GW * 3;
        uint32_t *out = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) {
            int X = p148_xi[x], fx = p148_fx[x], c[3];
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

void pattern_148(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    if (!p148_ready) p148_init();
    if (p148_last < 0 || (sl == 0 && p148_last != 0)) memset(p148_can, 0, sizeof p148_can);
    p148_last = sl;

    const float t = (float)frame;
    const float sd = (float)(seed & 255u) * 0.0245437f;

    for (int i = 0; i < P148_N * 3; i++) p148_can[i] *= 0.955f;

    const float K = 18.0f;
    const float T = 7200.0f;                       /* full cycle, 2 minutes */
    const float amp = 0.85f + 0.18f * sinf(t * 0.00027f + sd);   /* swing angle */
    const float lmax = 0.620f * (float)P148_GH;
    const int cidx = (int)(t * 1.0f) + (int)(seed & 8191u);
    const float railT = 0.055f * (float)P148_GH;
    const float railB = (float)P148_GH - railT;
    const float x0 = 0.055f * (float)P148_GW;
    const float dxp = (0.89f * (float)P148_GW) / (float)(P148_NP - 1);

    for (int i = 0; i < P148_NP; i++) {
        float n = K + (float)i;
        float wi = 6.28318531f * n / T;
        float L = lmax * sqrtf(K / n);
        float px = x0 + dxp * (float)i;

        uint32_t col = pal[(uint32_t)(cidx + i * 780) & JD_PAL_MASK];
        float r = (float)((col >> 16) & 255u) * (1.0f / 255.0f);
        float g = (float)((col >> 8) & 255u) * (1.0f / 255.0f);
        float b = (float)(col & 255u) * (1.0f / 255.0f);
        float mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 0.28f) mx = 0.28f;
        float nrm = 1.0f / mx;                 /* keep bobs vivid in dark ramps */
        r *= nrm; g *= nrm; b *= nrm;

        for (int m = 0; m < 2; m++) {
            float py = m ? railB : railT;
            float sgn = m ? -1.0f : 1.0f;
            /* three sub-frame stamps so the ribbon is continuous, not dashed */
            for (int q = 0; q < 3; q++) {
                float tt = t - (float)q * 0.333f;
                float th = amp * sinf(wi * tt + sd * 0.7f);
                float bx = px + L * sinf(th), by = py + sgn * L * cosf(th);
                if (q == 0)
                    for (int s2 = 1; s2 < 34; s2++) {
                        float u = (float)s2 * (1.0f / 34.0f);
                        p148_pt(px + (bx - px) * u, py + (by - py) * u,
                                r, g, b, 0.55f * u);
                    }
                p148_dot(bx, by, r, g, b, 0.30f);
            }
        }
    }

    const float *ca = p148_can;
    uint8_t *o = p148_img;
    for (int i = 0; i < P148_N * 3; i++) {
        int v = (int)(ca[i] * 70.0f);
        if (v > 1023) v = 1023;
        if (v < 0) v = 0;
        o[i] = p148_tone[v];
    }
    p148_upscale(fb, w, h);
}
