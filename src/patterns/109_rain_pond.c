/* 109 Rain Pond — an actual 2D wave equation, raindrop by raindrop.
 * A 320x240 height field is stepped with the discrete wave equation
 *   u' = 2u - u_prev + C.laplacian(u),  C = 0.30, damped 0.4% a frame
 * so ripples spread at a true constant speed, pass through each other,
 * interfere into rosettes and reflect off the rim of the pond. Drops land on a
 * Poisson-ish clock, each one a small Gaussian dent poured in over eight frames
 * so nothing ever pops. The surface is lit by a low sun: the shading term is
 * the dot product of the surface slope with the light, raised to a high power,
 * which paints thin blazing crest lines and leaves the troughs near-black —
 * sparse, and unlike any analytic ripple because the state is genuinely
 * integrated rather than evaluated. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p109_up;

#define P109_LW 320
#define P109_LH 240

static float p109_a[P109_LW * P109_LH];
static float p109_b[P109_LW * P109_LH];
static float *p109_cur = p109_a, *p109_prv = p109_b;
static uint8_t p109_img[P109_LW * P109_LH * 3];
static int *p109_xm;
static int p109_xm_w;
static uint8_t p109_spec[2048];
static uint8_t p109_ramp[256][3];
static uint32_t p109_rs = 0x109D40Fu;
static int p109_ready;
static int p109_drop_t;
static float p109_dx, p109_dy, p109_damp;
static int p109_dn;

static uint32_t p109_rnd(void)
{
    p109_rs ^= p109_rs << 13; p109_rs ^= p109_rs >> 17; p109_rs ^= p109_rs << 5;
    return p109_rs;
}

static void p109_init(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float s = (float)i * (1.0f / 2047.0f);
        float v = s * s;                       /* ^4 total with the caller's ^2 */
        p109_spec[i] = (uint8_t)(v * v * 255.0f);
    }
    p109_drop_t = 30;
    p109_ready = 1;
}

static void p109_build_ramp(const uint32_t *pal, int base)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * 128) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p109_ramp[i][0] = p109_ramp[i-1][0];
                     p109_ramp[i][1] = p109_ramp[i-1][1];
                     p109_ramp[i][2] = p109_ramp[i-1][2]; }
            else   { p109_ramp[i][0] = p109_ramp[i][1] = p109_ramp[i][2] = 210; }
            continue;
        }
        p109_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p109_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p109_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static void p109_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p109_xm_w != w) {
        free(p109_xm);
        p109_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p109_xm[x] = (int)(((long long)x * (P109_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p109_xm_w = w;
    }
    jd_up_blit(&p109_up, fb, w, h, p109_img, P109_LW, P109_LH);
}

void pattern_109(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float lx, ly, amb;
    int x, y, hbase;
    (void)sl;

    if (!p109_ready) p109_init();
    hbase = (int)(t * 1.2f) + (int)(seed & 32767);
    p109_build_ramp(pal, hbase);

    /* --- drops: a new one every 34..118 frames, poured over 8 frames --- */
    if (p109_dn > 0) {
        int cx = (int)p109_dx, cy = (int)p109_dy, i, j;
        for (j = -8; j <= 8; j++)
            for (i = -8; i <= 8; i++) {
                int px = cx + i, py = cy + j;
                float d2 = (float)(i * i + j * j);
                if ((unsigned)px >= P109_LW || (unsigned)py >= P109_LH) continue;
                p109_cur[py * P109_LW + px] -= p109_damp * expf(-d2 * 0.055f);
            }
        p109_dn--;
    } else if (--p109_drop_t <= 0) {
        uint32_t r = p109_rnd() ^ seed;
        p109_dx = 22.0f + (float)((r >> 3) % (P109_LW - 44));
        p109_dy = 20.0f + (float)((r >> 15) % (P109_LH - 40));
        p109_damp = 0.115f + (float)((r >> 27) & 7) * 0.016f;
        p109_dn = 8;
        p109_drop_t = 55 + (int)((p109_rnd() >> 9) % 120);
    }

    /* --- wave step: new state is written into the old-previous buffer, then
     * the two buffers swap roles, so the laplacian always reads one clean
     * snapshot (an in-place update would bias the stencil sideways) --- */
    for (y = 1; y < P109_LH - 1; y++) {
        const float *u = p109_cur + y * P109_LW;
        float *p = p109_prv + y * P109_LW;
        const float *un = u - P109_LW, *us = u + P109_LW;
        for (x = 1; x < P109_LW - 1; x++) {
            float c = u[x];
            float lap = u[x - 1] + u[x + 1] + un[x] + us[x] - 4.0f * c;
            p[x] = (2.0f * c - p[x] + 0.30f * lap) * 0.9955f;
        }
    }
    { float *tmp = p109_cur; p109_cur = p109_prv; p109_prv = tmp; }

    /* --- shade --- */
    {
        float la = t * 0.00052f;
        lx = cosf(la) * 0.72f; ly = sinf(la) * 0.72f;
        amb = 0.30f;
    }
    for (y = 0; y < P109_LH; y++) {
        const float *u = p109_cur + y * P109_LW;
        const float *un = u - (y > 0 ? P109_LW : 0);
        const float *us = u + (y < P109_LH - 1 ? P109_LW : 0);
        uint8_t *row = p109_img + y * P109_LW * 3;
        for (x = 0; x < P109_LW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < P109_LW - 1 ? x + 1 : P109_LW - 1;
            float gx = (u[xp] - u[xm]) * 8.0f;
            float gy = (us[x] - un[x]) * 8.0f;
            float d = gx * lx + gy * ly + amb;
            float g2 = gx * gx + gy * gy;
            int si, lum, hue, o = x * 3;
            const uint8_t *cp;
            if (d < 0.0f) d = 0.0f;
            si = (int)(d * 540.0f);
            if (si > 2047) si = 2047;
            lum = p109_spec[si] + (int)(g2 * 85.0f);
            if (lum > 255) lum = 255;
            hue = (hbase / 20 + (int)(u[x] * 21.0f) + (int)(g2 * 9.0f)) & 255;
            cp = p109_ramp[hue];
            row[o + 0] = (uint8_t)(4 + ((cp[0] * lum) >> 8) - ((4 * lum) >> 8));
            row[o + 1] = (uint8_t)(8 + ((cp[1] * lum) >> 8) - ((8 * lum) >> 8));
            row[o + 2] = (uint8_t)(14 + ((cp[2] * lum) >> 8) - ((14 * lum) >> 8));
        }
    }
    p109_blit(fb, w, h);
}
