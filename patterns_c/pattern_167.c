/* 167 Medusa Drift — bioluminescent jellyfish, which is what the app is named
 * for. Four medusae drift on slow Lissajous paths. Each bell is a parametric
 * dome that CONTRACTS and relaxes on its own period — wide and flat on the
 * push, tall and narrow on the recovery — with radial canals drawn inside it
 * and a scalloped margin. Behind each bell trail six tentacles and four oral
 * arms, each a travelling-wave filament whose amplitude grows toward the tip
 * and whose phase lags the bell's contraction, so the whole animal reads as
 * one soft pulse. Drawn into a decaying plate, so everything glows.
 * The plate is deliberately never cleared on an sl discontinuity: it refreshes
 * itself within ~30 frames, so clearing it would only add a hard cut. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p167_up;

#define P167_LW 480
#define P167_LH 360
#define P167_TAU 6.28318530717958647692f

static uint8_t p167_img[P167_LW * P167_LH * 3];
static int    *p167_xm;
static int     p167_xm_w;
static uint8_t p167_ramp[256][3];

/* 256-entry hue ramp lifted from the live palette. Palette schemes contain
 * near-black stretches; a too-dark sample inherits its predecessor so the
 * figure never gets punched full of dark holes. */
static void p167_build_ramp(const uint32_t *pal, int base, int span)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * span) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p167_ramp[i][0] = p167_ramp[i-1][0];
                     p167_ramp[i][1] = p167_ramp[i-1][1];
                     p167_ramp[i][2] = p167_ramp[i-1][2]; }
            else   { p167_ramp[i][0] = p167_ramp[i][1] = p167_ramp[i][2] = 255; }
            continue;
        }
        p167_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p167_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p167_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

/* bilinear upscale of the low-res image onto the real framebuffer */
static void p167_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p167_xm_w != w) {
        free(p167_xm);
        p167_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p167_xm[x] = (int)(((long long)x * (P167_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p167_xm_w = w;
    }
    jd_up_blit(&p167_up, fb, w, h, p167_img, P167_LW, P167_LH);
}

static float p167_acc[P167_LW * P167_LH * 3];

/* bilinear additive splat */
static void p167_splat(float x, float y, float r, float g, float b)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w00, w01, w10, w11;
    float *q;
    if (xi < 0 || yi < 0 || xi >= P167_LW - 1 || yi >= P167_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy); w01 = fx * (1.0f - fy);
    w10 = (1.0f - fx) * fy;          w11 = fx * fy;
    q = p167_acc + (yi * P167_LW + xi) * 3;
    q[0] += r * w00; q[1] += g * w00; q[2] += b * w00;
    q[3] += r * w01; q[4] += g * w01; q[5] += b * w01;
    q += P167_LW * 3;
    q[0] += r * w10; q[1] += g * w10; q[2] += b * w10;
    q[3] += r * w11; q[4] += g * w11; q[5] += b * w11;
}

/* soft 5-tap stroke: bilinear core plus a dim cross, so a one-pixel figure
 * survives the upscale as a glowing filament instead of a hairline */
static void p167_stroke(float x, float y, float r, float g, float b)
{
    p167_splat(x, y, r, g, b);
    r *= 0.30f; g *= 0.30f; b *= 0.30f;
    p167_splat(x - 1.6f, y, r, g, b);
    p167_splat(x + 1.6f, y, r, g, b);
    p167_splat(x, y - 1.6f, r, g, b);
    p167_splat(x, y + 1.6f, r, g, b);
}

/* one pass: tone-map the accumulator into the image and decay it. The decay is
 * what turns per-frame stamps into smooth trails and keeps frame-to-frame
 * delta low — nothing in this pattern can appear or vanish in one frame. */
static void p167_tone(float decay)
{
    int i, n = P167_LW * P167_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p167_acc[i];
        int c = (int)(v * 255.0f);
        p167_img[i] = c <= 0 ? 0 : c >= 255 ? 255 : (uint8_t)c;
        p167_acc[i] = v * decay;
    }
}


#define P167_NM 4
#define P167_TENT 6
#define P167_ARMS 4


static void p167_seg(float x0, float y0, float x1, float y1,
                     const uint8_t *cp, float g0, float g1)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int steps = (int)(len * 0.9f) + 1, k;
    float inv = 1.0f / (float)steps;
    for (k = 0; k < steps; k++) {
        float u = (float)k * inv;
        float g = (g0 + (g1 - g0) * u) * (1.0f / 255.0f);
        p167_stroke(x0 + dx * u, y0 + dy * u,
                    cp[0] * g, cp[1] * g, cp[2] * g);
    }
}

void pattern_167(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float s = (float)(seed & 1023) * 0.00613f;
    int m, i, hb;

    (void)sl;
    p167_build_ramp(pal, (int)(t * 1.5f) + (int)(seed & 32767), 84);
    hb = (int)(t * 0.045f);

    for (m = 0; m < P167_NM; m++) {
        float mf = (float)m;
        float sp = s + mf * 1.77f;
        float cx = 240.0f + 150.0f * sinf(t * (0.00021f + 0.00004f * mf) + sp);
        float cy = 128.0f + 74.0f * sinf(t * (0.00013f + 0.00005f * mf) + sp * 1.6f);
        float R  = 30.0f + 12.0f * sinf(sp * 2.1f);
        float pulse = 0.5f + 0.5f * sinf(t * (0.0135f + 0.0021f * mf) + sp * 3.0f);
        float bw = R * (0.84f + 0.30f * pulse);
        float bh = R * (1.30f - 0.42f * pulse);
        const uint8_t *cbell = p167_ramp[(hb + (int)(mf * 52.0f)) & 255];
        const uint8_t *crim  = p167_ramp[(hb + (int)(mf * 52.0f) + 22) & 255];
        const uint8_t *ctent = p167_ramp[(hb + (int)(mf * 52.0f) + 40) & 255];
        float px = 0.0f, py = 0.0f;

        cy += 9.0f * pulse;                       /* the bell bobs as it pushes */

        /* the bell: a dome with a scalloped margin */
        for (i = 0; i <= 64; i++) {
            float a = (float)i * (3.14159265f / 64.0f);
            float sc = 1.0f + 0.055f * sinf(a * 8.0f + t * 0.006f);
            float x = cx + bw * cosf(a) * sc;
            float y = cy - bh * sinf(a) * sc;
            if (i) p167_seg(px, py, x, y, cbell, 0.30f, 0.30f);
            px = x; py = y;
        }
        /* the margin itself, brighter, with a slight droop */
        for (i = 0; i <= 40; i++) {
            float u = (float)i * (1.0f / 40.0f);
            float q = 2.0f * u - 1.0f;
            float x = cx + bw * q;
            float y = cy + (1.0f - q * q) * (5.0f + 3.0f * pulse)
                         + 2.6f * sinf(u * 12.56637f + t * 0.010f + sp);
            if (i) p167_seg(px, py, x, y, crim, 0.46f, 0.46f);
            px = x; py = y;
        }
        /* radial canals inside the bell */
        for (i = 0; i < 5; i++) {
            float a = 0.32f + (float)i * (2.5f / 4.0f);
            p167_seg(cx + bw * cosf(a) * 0.94f, cy - bh * sinf(a) * 0.94f,
                     cx + bw * cosf(a) * 0.12f, cy - bh * 0.10f,
                     cbell, 0.26f, 0.06f);
        }
        /* tentacles: travelling waves whose phase lags the bell */
        for (i = 0; i < P167_TENT; i++) {
            float u0 = ((float)i - (float)(P167_TENT - 1) * 0.5f) / (float)(P167_TENT - 1);
            float x0 = cx + u0 * bw * 1.85f;
            float L  = 96.0f + 46.0f * sinf(sp + (float)i * 1.3f);
            int k;
            px = x0; py = cy;
            for (k = 1; k <= 22; k++) {
                float v = (float)k * (1.0f / 22.0f);
                float x = x0 + (10.0f + 20.0f * v) * v *
                          sinf(v * 5.3f - t * 0.021f + (float)i * 1.1f + sp);
                float y = cy + L * v;
                p167_seg(px, py, x, y, ctent, 0.34f * (1.0f - v * 0.85f),
                                              0.34f * (1.0f - (v + 0.05f) * 0.85f));
                px = x; py = y;
            }
        }
        /* oral arms: shorter, frillier, closer in */
        for (i = 0; i < P167_ARMS; i++) {
            float u0 = ((float)i - 1.5f) * 0.36f;
            float x0 = cx + u0 * bw;
            int k;
            px = x0; py = cy;
            for (k = 1; k <= 14; k++) {
                float v = (float)k * (1.0f / 14.0f);
                float x = x0 + (7.0f + 16.0f * v) * v *
                          sinf(v * 9.0f - t * 0.017f + (float)i * 2.2f + sp * 0.7f);
                float y = cy + (46.0f + 8.0f * pulse) * v;
                p167_seg(px, py, x, y, crim, 0.30f * (1.0f - v * 0.7f),
                                             0.30f * (1.0f - (v + 0.07f) * 0.7f));
                px = x; py = y;
            }
        }
    }
    p167_tone(0.86f);
    p167_blit(fb, w, h);
}
