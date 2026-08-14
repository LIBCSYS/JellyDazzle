/* 102 Vortex Silk — seven point vortices stirring a sheet of tracer threads.
 * This is the classical 2-D point-vortex problem, integrated for real: each
 * vortex is carried by the flow of all the others,
 *   v(p) = sum_j  G_j * (-(p-p_j).y, (p-p_j).x) / (|p-p_j|^2 + e),
 * which is why the pairs leapfrog, the triples orbit, and opposite-sign pairs
 * shoot across the frame like smoke rings — none of that is animated by hand.
 * 3000 massless tracers ride the same field and leave decaying trails, so the
 * flow draws itself as silk: threads wind onto the cores, stretch into long
 * filaments between them and fold along the separatrices. A rolling slice of
 * the tracer population is re-seeded every frame so the sheet never runs out.
 * Trails on black — sparse, and the motion is pure advection, so it is smooth
 * by construction. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p102_up;

#define P102_LW 480
#define P102_LH 360
#define P102_NV 7
#define P102_NT 1000

static float p102_acc[P102_LW * P102_LH * 3];
static uint8_t p102_img[P102_LW * P102_LH * 3];
static int *p102_xm;
static int p102_xm_w;
static uint8_t p102_tone[4096];
static uint8_t p102_ramp[256][3];
static float p102_vx[P102_NV], p102_vy[P102_NV], p102_g[P102_NV];
static float p102_tx[P102_NT], p102_ty[P102_NT];
static uint8_t p102_th[P102_NT];
static uint32_t p102_rs = 0x102CE1F3u;
static int p102_ready;

static uint32_t p102_rnd(void)
{
    p102_rs ^= p102_rs << 13; p102_rs ^= p102_rs >> 17; p102_rs ^= p102_rs << 5;
    return p102_rs;
}
static float p102_r01(void) { return (float)(p102_rnd() >> 12 & 4095) * (1.0f / 4095.0f); }

static void p102_seed_tracer(int i)
{
    p102_tx[i] = p102_r01() * P102_LW;
    p102_ty[i] = p102_r01() * P102_LH;
    p102_th[i] = (uint8_t)(p102_rnd() >> 21);
}

static void p102_init(void)
{
    int i;
    for (i = 0; i < 4096; i++) {
        float v = 1.0f - expf(-(float)i * (3.6f / 4096.0f));
        p102_tone[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    for (i = 0; i < P102_NV; i++) {
        float a = (float)i * (6.2831853f / (float)P102_NV);
        p102_vx[i] = P102_LW * 0.5f + 108.0f * cosf(a);
        p102_vy[i] = P102_LH * 0.5f + 78.0f * sinf(a);
        p102_g[i] = ((i & 1) ? -1.0f : 1.0f) * (62.0f + 20.0f * (float)(i % 3));
    }
    for (i = 0; i < P102_NT; i++) p102_seed_tracer(i);
    p102_ready = 1;
}

static void p102_build_ramp(const uint32_t *pal, int base)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * 128) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p102_ramp[i][0] = p102_ramp[i-1][0];
                     p102_ramp[i][1] = p102_ramp[i-1][1];
                     p102_ramp[i][2] = p102_ramp[i-1][2]; }
            else   { p102_ramp[i][0] = p102_ramp[i][1] = p102_ramp[i][2] = 210; }
            continue;
        }
        p102_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p102_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p102_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static void p102_dot(float x, float y, const uint8_t *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, *p;
    float r = c[0] * wgt, g = c[1] * wgt, b = c[2] * wgt;
    if ((unsigned)xi >= P102_LW - 1 || (unsigned)yi >= P102_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p102_acc + (yi * P102_LW + xi) * 3;
    {
        float w00 = (1.0f - fx) * (1.0f - fy), w10 = fx * (1.0f - fy);
        float w01 = (1.0f - fx) * fy, w11 = fx * fy;
        p[0] += r * w00; p[1] += g * w00; p[2] += b * w00;
        p[3] += r * w10; p[4] += g * w10; p[5] += b * w10;
        p += P102_LW * 3;
        p[0] += r * w01; p[1] += g * w01; p[2] += b * w01;
        p[3] += r * w11; p[4] += g * w11; p[5] += b * w11;
    }
}

static void p102_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p102_xm_w != w) {
        free(p102_xm);
        p102_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p102_xm[x] = (int)(((long long)x * (P102_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p102_xm_w = w;
    }
    jd_up_blit(&p102_up, fb, w, h, p102_img, P102_LW, P102_LH);
}

void pattern_102(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float nvx[P102_NV], nvy[P102_NV];
    int i, j, n3 = P102_LW * P102_LH * 3, hbase;
    const float cx = P102_LW * 0.5f, cy = P102_LH * 0.5f;
    (void)sl; (void)seed;

    if (!p102_ready) p102_init();
    hbase = (int)(t * 1.4f) + (int)(seed & 32767);
    p102_build_ramp(pal, hbase);

    /* --- vortices carry each other; a weak central spring keeps them on stage */
    for (i = 0; i < P102_NV; i++) {
        float ux = 0.0f, uy = 0.0f;
        for (j = 0; j < P102_NV; j++) {
            float dx, dy, r2, k;
            if (j == i) continue;
            dx = p102_vx[i] - p102_vx[j];
            dy = p102_vy[i] - p102_vy[j];
            r2 = dx * dx + dy * dy + 900.0f;
            k = p102_g[j] / r2;
            ux -= dy * k; uy += dx * k;
        }
        ux -= (p102_vx[i] - cx) * 0.00055f;
        uy -= (p102_vy[i] - cy) * 0.00075f;
        nvx[i] = p102_vx[i] + ux;
        nvy[i] = p102_vy[i] + uy;
    }
    for (i = 0; i < P102_NV; i++) { p102_vx[i] = nvx[i]; p102_vy[i] = nvy[i]; }

    /* --- tracers ride the same field --- */
    for (i = 0; i < P102_NT; i++) {
        float px = p102_tx[i], py = p102_ty[i];
        float ux = 0.0f, uy = 0.0f, sp;
        for (j = 0; j < P102_NV; j++) {
            float dx = px - p102_vx[j], dy = py - p102_vy[j];
            float r2 = dx * dx + dy * dy + 260.0f;
            float k = p102_g[j] / r2;
            ux -= dy * k; uy += dx * k;
        }
        px += ux; py += uy;
        if (px < -12.0f || px > P102_LW + 12.0f ||
            py < -12.0f || py > P102_LH + 12.0f) { p102_seed_tracer(i); continue; }
        p102_tx[i] = px; p102_ty[i] = py;
        sp = ux * ux + uy * uy;
        {
            /* brightness follows local speed: the slow far field fades out and
             * the fast collar around each core burns, so the picture is a map
             * of the flow's energy rather than a uniform fur */
            float v = sqrtf(sp);
            float wg = 0.0016f + 0.0125f * (v > 2.4f ? 2.4f : v);
            int hue = (hbase / 15 + p102_th[i] / 5 + (int)(v * 26.0f)) & 255;
            p102_dot(px, py, p102_ramp[hue], wg);
        }
    }
    for (i = 0; i < 7; i++)                     /* rolling re-seed */
        p102_seed_tracer((frame * 7 + i) % P102_NT);

    /* --- vortex cores glow faintly --- */
    for (j = 0; j < P102_NV; j++) {
        const uint8_t *cp = p102_ramp[(hbase / 15 + j * 30) & 255];
        p102_dot(p102_vx[j], p102_vy[j], cp, 0.9f);
        p102_dot(p102_vx[j] + 1.4f, p102_vy[j], cp, 0.45f);
        p102_dot(p102_vx[j] - 1.4f, p102_vy[j], cp, 0.45f);
        p102_dot(p102_vx[j], p102_vy[j] + 1.4f, cp, 0.45f);
        p102_dot(p102_vx[j], p102_vy[j] - 1.4f, cp, 0.45f);
    }

    for (i = 0; i < n3; i++) {
        float v = p102_acc[i];
        int ti = (int)(v * 900.0f);
        if (ti > 4095) ti = 4095;
        p102_img[i] = p102_tone[ti];
        p102_acc[i] = v * 0.977f;
    }
    p102_blit(fb, w, h);
}
