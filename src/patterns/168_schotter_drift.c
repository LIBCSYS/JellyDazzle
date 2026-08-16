/* 168 Schotter Drift — Georg Nees' 1968 plotter piece "Schotter" (gravel), set
 * in motion. A perfectly ruled grid of squares at the top gradually loses its
 * discipline toward the bottom: each cell is rotated and displaced by an amount
 * that grows with depth. In the original the disorder came from a random number
 * generator; here every cell has its own slow sinusoid, so the gravel wanders
 * continuously instead of jittering, and a wave travelling down the grid moves
 * the boundary between order and chaos. Outlines only, on black.
 * The plate is deliberately never cleared on an sl discontinuity: it refreshes
 * itself within ~30 frames, so clearing it would only add a hard cut. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p168_up;

#define P168_LW 480
#define P168_LH 360
#define P168_TAU 6.28318530717958647692f

static uint8_t p168_img[P168_LW * P168_LH * 3];
static int    *p168_xm;
static int     p168_xm_w;
static uint8_t p168_ramp[256][3];

/* 256-entry hue ramp lifted from the live palette. Palette schemes contain
 * near-black stretches; a too-dark sample inherits its predecessor so the
 * figure never gets punched full of dark holes. */
static void p168_build_ramp(const uint32_t *pal, int base, int span)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * span) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p168_ramp[i][0] = p168_ramp[i-1][0];
                     p168_ramp[i][1] = p168_ramp[i-1][1];
                     p168_ramp[i][2] = p168_ramp[i-1][2]; }
            else   { p168_ramp[i][0] = p168_ramp[i][1] = p168_ramp[i][2] = 255; }
            continue;
        }
        p168_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p168_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p168_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

/* bilinear upscale of the low-res image onto the real framebuffer */
static void p168_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p168_xm_w != w) {
        free(p168_xm);
        p168_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p168_xm[x] = (int)(((long long)x * (P168_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p168_xm_w = w;
    }
    jd_up_blit(&p168_up, fb, w, h, p168_img, P168_LW, P168_LH);
}

static float p168_acc[P168_LW * P168_LH * 3];

/* bilinear additive splat */
static void p168_splat(float x, float y, float r, float g, float b)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w00, w01, w10, w11;
    float *q;
    if (xi < 0 || yi < 0 || xi >= P168_LW - 1 || yi >= P168_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy); w01 = fx * (1.0f - fy);
    w10 = (1.0f - fx) * fy;          w11 = fx * fy;
    q = p168_acc + (yi * P168_LW + xi) * 3;
    q[0] += r * w00; q[1] += g * w00; q[2] += b * w00;
    q[3] += r * w01; q[4] += g * w01; q[5] += b * w01;
    q += P168_LW * 3;
    q[0] += r * w10; q[1] += g * w10; q[2] += b * w10;
    q[3] += r * w11; q[4] += g * w11; q[5] += b * w11;
}

/* soft 5-tap stroke: bilinear core plus a dim cross, so a one-pixel figure
 * survives the upscale as a glowing filament instead of a hairline */
static void p168_stroke(float x, float y, float r, float g, float b)
{
    p168_splat(x, y, r, g, b);
    r *= 0.30f; g *= 0.30f; b *= 0.30f;
    p168_splat(x - 1.6f, y, r, g, b);
    p168_splat(x + 1.6f, y, r, g, b);
    p168_splat(x, y - 1.6f, r, g, b);
    p168_splat(x, y + 1.6f, r, g, b);
}

/* one pass: tone-map the accumulator into the image and decay it. The decay is
 * what turns per-frame stamps into smooth trails and keeps frame-to-frame
 * delta low — nothing in this pattern can appear or vanish in one frame. */
static void p168_tone(float decay)
{
    int i, n = P168_LW * P168_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p168_acc[i];
        int c = (int)(v * 255.0f);
        p168_img[i] = c <= 0 ? 0 : c >= 255 ? 255 : (uint8_t)c;
        p168_acc[i] = v * decay;
    }
}


#define P168_GX 15
#define P168_GY 13
#define P168_N  (P168_GX * P168_GY)

static float p168_ph[P168_N * 3], p168_om[P168_N * 3];
static int   p168_ready;

static void p168_seg(float x0, float y0, float x1, float y1,
                     const uint8_t *cp, float g)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int steps = (int)(len * 0.85f) + 1, k;
    float inv = 1.0f / (float)steps;
    g *= (1.0f / 255.0f);
    for (k = 0; k < steps; k++) {
        float u = (float)k * inv;
        p168_stroke(x0 + dx * u, y0 + dy * u, cp[0] * g, cp[1] * g, cp[2] * g);
    }
}

static void p168_init(uint32_t seed)
{
    uint32_t r = 0x9E3779B9u ^ seed;
    int i;
    for (i = 0; i < P168_N * 3; i++) {
        r ^= r << 13; r ^= r >> 17; r ^= r << 5;
        p168_ph[i] = (float)(r >> 8) * (1.0f / 16777216.0f) * 6.2831853f;
        r ^= r << 13; r ^= r >> 17; r ^= r << 5;
        p168_om[i] = 0.0045f + (float)(r >> 8) * (1.0f / 16777216.0f) * 0.0075f;
    }
    p168_ready = 1;
}

void pattern_168(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float s = (float)(seed & 1023) * 0.00613f;
    float cell, half, x0, y0, amp, wave;
    int gx, gy, hb;

    (void)sl;
    if (!p168_ready) p168_init(seed);
    p168_build_ramp(pal, (int)(t * 4.1f) + (int)(seed & 32767), 70);
    hb = 0;  /* TEMPORAL REVIEW 2.4.0 (F-16x): the discrete hb hue
             * stepper snapped the whole figure one ramp entry at a
             * time; its rate is folded into the ramp base above,
             * which slides continuously. */

    cell = 25.0f;
    half = cell * 0.36f;
    x0   = 240.0f - (float)(P168_GX - 1) * cell * 0.5f;
    y0   = 180.0f - (float)(P168_GY - 1) * cell * 0.5f;
    amp  = 0.62f + 0.38f * sinf(t * 0.00046f + s);
    wave = 0.5f + 0.5f * sinf(t * 0.00028f + s * 1.4f);   /* order/chaos front */

    for (gy = 0; gy < P168_GY; gy++) {
        float v = (float)gy / (float)(P168_GY - 1);
        float d = v - 0.22f * wave;
        float dis;
        if (d < 0.0f) d = 0.0f;
        dis = powf(d, 1.75f) * amp;
        for (gx = 0; gx < P168_GX; gx++) {
            int i = (gy * P168_GX + gx) * 3, k;
            float th = dis * 1.35f * sinf(p168_ph[i]   + t * p168_om[i]);
            float ox = dis * 11.0f * sinf(p168_ph[i+1] + t * p168_om[i+1]);
            float oy = dis * 11.0f * sinf(p168_ph[i+2] + t * p168_om[i+2]);
            float cx = x0 + (float)gx * cell + ox;
            float cy = y0 + (float)gy * cell + oy;
            float cs = cosf(th), sn = sinf(th);
            float px = 0.0f, py = 0.0f;
            const uint8_t *cp = p168_ramp[(hb + gx * 6 + gy * 3) & 255];
            float g = 0.30f * (1.0f - 0.30f * dis);
            for (k = 0; k <= 4; k++) {
                float ax = (k == 0 || k == 3 || k == 4) ? -half : half;
                float ay = (k < 2 || k == 4) ? -half : half;
                float x = cx + ax * cs - ay * sn;
                float y = cy + ax * sn + ay * cs;
                if (k) p168_seg(px, py, x, y, cp, g);
                px = x; py = y;
            }
        }
    }
    p168_tone(0.72f);
    p168_blit(fb, w, h);
}
