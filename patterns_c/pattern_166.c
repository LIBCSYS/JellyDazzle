/* 166 Star Trails — a long exposure pointed at the celestial pole.
 * Nine hundred stars turn about an off-centre pole at one revolution every two
 * and a half minutes, and the frame is an exposure that never closes: every
 * star adds light where it is and the whole plate fades exponentially, so each
 * star drags a concentric arc behind it whose length is set by the fade rather
 * than by any drawn geometry. Stars near the pole crawl and read brightest;
 * stars far out sweep and thin. On entry the exposure is primed with six
 * hundred frames of history so the arcs are already there. Sparse on black. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p166_up;

#define P166_LW 480
#define P166_LH 360
#define P166_TAU 6.28318530717958647692f

static uint8_t p166_img[P166_LW * P166_LH * 3];
static int    *p166_xm;
static int     p166_xm_w;
static uint8_t p166_ramp[256][3];

/* 256-entry hue ramp lifted from the live palette. Palette schemes contain
 * near-black stretches; a too-dark sample inherits its predecessor so the
 * figure never gets punched full of dark holes. */
static void p166_build_ramp(const uint32_t *pal, int base, int span)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * span) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p166_ramp[i][0] = p166_ramp[i-1][0];
                     p166_ramp[i][1] = p166_ramp[i-1][1];
                     p166_ramp[i][2] = p166_ramp[i-1][2]; }
            else   { p166_ramp[i][0] = p166_ramp[i][1] = p166_ramp[i][2] = 255; }
            continue;
        }
        p166_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p166_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p166_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

/* bilinear upscale of the low-res image onto the real framebuffer */
static void p166_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p166_xm_w != w) {
        free(p166_xm);
        p166_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p166_xm[x] = (int)(((long long)x * (P166_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p166_xm_w = w;
    }
    jd_up_blit(&p166_up, fb, w, h, p166_img, P166_LW, P166_LH);
}

static float p166_acc[P166_LW * P166_LH * 3];

/* bilinear additive splat */
static void p166_splat(float x, float y, float r, float g, float b)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w00, w01, w10, w11;
    float *q;
    if (xi < 0 || yi < 0 || xi >= P166_LW - 1 || yi >= P166_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy); w01 = fx * (1.0f - fy);
    w10 = (1.0f - fx) * fy;          w11 = fx * fy;
    q = p166_acc + (yi * P166_LW + xi) * 3;
    q[0] += r * w00; q[1] += g * w00; q[2] += b * w00;
    q[3] += r * w01; q[4] += g * w01; q[5] += b * w01;
    q += P166_LW * 3;
    q[0] += r * w10; q[1] += g * w10; q[2] += b * w10;
    q[3] += r * w11; q[4] += g * w11; q[5] += b * w11;
}

/* soft 5-tap stroke: bilinear core plus a dim cross, so a one-pixel figure
 * survives the upscale as a glowing filament instead of a hairline */
static void p166_stroke(float x, float y, float r, float g, float b)
{
    p166_splat(x, y, r, g, b);
    r *= 0.30f; g *= 0.30f; b *= 0.30f;
    p166_splat(x - 1.6f, y, r, g, b);
    p166_splat(x + 1.6f, y, r, g, b);
    p166_splat(x, y - 1.6f, r, g, b);
    p166_splat(x, y + 1.6f, r, g, b);
}

/* one pass: tone-map the accumulator into the image and decay it. The decay is
 * what turns per-frame stamps into smooth trails and keeps frame-to-frame
 * delta low — nothing in this pattern can appear or vanish in one frame. */
static void p166_tone(float decay)
{
    int i, n = P166_LW * P166_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p166_acc[i];
        int c = (int)(v * 255.0f);
        p166_img[i] = c <= 0 ? 0 : c >= 255 ? 255 : (uint8_t)c;
        p166_acc[i] = v * decay;
    }
}


#define P166_NS 900
#define P166_OMEGA (P166_TAU / 9000.0f)
#define P166_DECAY 0.9976f

static float   p166_r[P166_NS], p166_a[P166_NS], p166_m[P166_NS];
static uint8_t p166_hue[P166_NS], p166_spk[P166_NS];
static int     p166_ready, p166_primed;
static uint32_t p166_rs = 0x5EED1234u;

static float p166_rf(void)
{
    p166_rs ^= p166_rs << 13; p166_rs ^= p166_rs >> 17; p166_rs ^= p166_rs << 5;
    return (float)(p166_rs >> 8) * (1.0f / 16777216.0f);
}

static void p166_init(uint32_t seed)
{
    int i;
    p166_rs = 0x5EED1234u ^ seed;
    for (i = 0; i < P166_NS; i++) {
        float u = p166_rf();
        p166_r[i]   = sqrtf(u) * 300.0f;
        p166_a[i]   = p166_rf() * P166_TAU;
        u           = p166_rf();
        p166_m[i]   = 0.16f + 0.84f * u * u * u;       /* few bright, many faint */
        p166_hue[i] = (uint8_t)(p166_rf() * 255.0f);
        p166_spk[i] = p166_m[i] > 0.62f ? 1 : 0;
    }
    p166_ready = 1;
}

/* one exposure step: every star deposits at its position for time tt */
static void p166_step(float tt, float cx, float cy, float gain, int hb)
{
    int i;
    for (i = 0; i < P166_NS; i++) {
        float an = p166_a[i] + tt * P166_OMEGA;
        float x = cx + p166_r[i] * cosf(an) * 1.28f;
        float y = cy + p166_r[i] * sinf(an);
        float g = p166_m[i] * gain;
        const uint8_t *cp = p166_ramp[(hb + p166_hue[i]) & 255];
        float r = cp[0] * g * (1.0f / 255.0f);
        float gg = cp[1] * g * (1.0f / 255.0f);
        float b = cp[2] * g * (1.0f / 255.0f);
        if (p166_spk[i]) p166_stroke(x, y, r, gg, b);  /* bright: soft halo */
        else             p166_splat(x, y, r, gg, b);
    }
}

void pattern_166(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float cx, cy;
    int hb;

    (void)sl;
    if (!p166_ready) p166_init(seed);
    p166_build_ramp(pal, (int)(t * 1.1f) + (int)(seed & 32767), 76);
    hb = (int)(t * 0.03f);
    cx = 262.0f + 9.0f * sinf(t * 0.000071f);
    cy = 166.0f + 7.0f * cosf(t * 0.000059f);

    if (!p166_primed) {
        int k;
        p166_primed = 1;
        /* prime the plate: 600 frames of history, sampled every third frame */
        for (k = 200; k > 0; k--) {
            float age = (float)(k * 3);
            p166_step(t - age, cx, cy, 0.125f * 3.0f * powf(P166_DECAY, age), hb);
        }
    }
    p166_step(t, cx, cy, 0.125f, hb);
    p166_tone(P166_DECAY);
    p166_blit(fb, w, h);
}
