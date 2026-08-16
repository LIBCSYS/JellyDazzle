/* 165 Vortex Street — the von Karman wake behind a cylinder, drawn in dye.
 * A row of counter-rotating point vortices drifts downstream in the classic
 * staggered double row; 7200 massless tracer particles are pushed by
 * the exact Biot-Savart sum of those vortices plus the free stream, and the
 * streaklines they leave are the picture. Nothing is scripted — the curls,
 * pinches and hand-offs between vortices all fall out of the velocity field.
 * Vortex strength and particle brightness both ramp in and out at the domain
 * edges, so nothing ever appears or vanishes in a single frame. Dye on black.
 * The plate is deliberately never cleared on an sl discontinuity: it refreshes
 * itself within ~30 frames, so clearing it would only add a hard cut. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p165_up;

#define P165_LW 480
#define P165_LH 360
#define P165_TAU 6.28318530717958647692f

static uint8_t p165_img[P165_LW * P165_LH * 3];
static int    *p165_xm;
static int     p165_xm_w;
static uint8_t p165_ramp[256][3];

/* 256-entry hue ramp lifted from the live palette. Palette schemes contain
 * near-black stretches; a too-dark sample inherits its predecessor so the
 * figure never gets punched full of dark holes. */
static void p165_build_ramp(const uint32_t *pal, int base, int span)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * span) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p165_ramp[i][0] = p165_ramp[i-1][0];
                     p165_ramp[i][1] = p165_ramp[i-1][1];
                     p165_ramp[i][2] = p165_ramp[i-1][2]; }
            else   { p165_ramp[i][0] = p165_ramp[i][1] = p165_ramp[i][2] = 255; }
            continue;
        }
        p165_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p165_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p165_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

/* bilinear upscale of the low-res image onto the real framebuffer */
static void p165_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p165_xm_w != w) {
        free(p165_xm);
        p165_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p165_xm[x] = (int)(((long long)x * (P165_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p165_xm_w = w;
    }
    jd_up_blit(&p165_up, fb, w, h, p165_img, P165_LW, P165_LH);
}

static float p165_acc[P165_LW * P165_LH * 3];

/* bilinear additive splat */
static void p165_splat(float x, float y, float r, float g, float b)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w00, w01, w10, w11;
    float *q;
    if (xi < 0 || yi < 0 || xi >= P165_LW - 1 || yi >= P165_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy); w01 = fx * (1.0f - fy);
    w10 = (1.0f - fx) * fy;          w11 = fx * fy;
    q = p165_acc + (yi * P165_LW + xi) * 3;
    q[0] += r * w00; q[1] += g * w00; q[2] += b * w00;
    q[3] += r * w01; q[4] += g * w01; q[5] += b * w01;
    q += P165_LW * 3;
    q[0] += r * w10; q[1] += g * w10; q[2] += b * w10;
    q[3] += r * w11; q[4] += g * w11; q[5] += b * w11;
}

/* soft 5-tap stroke: bilinear core plus a dim cross, so a one-pixel figure
 * survives the upscale as a glowing filament instead of a hairline */
static void p165_stroke(float x, float y, float r, float g, float b)
{
    p165_splat(x, y, r, g, b);
    r *= 0.30f; g *= 0.30f; b *= 0.30f;
    p165_splat(x - 1.6f, y, r, g, b);
    p165_splat(x + 1.6f, y, r, g, b);
    p165_splat(x, y - 1.6f, r, g, b);
    p165_splat(x, y + 1.6f, r, g, b);
}

/* one pass: tone-map the accumulator into the image and decay it. The decay is
 * what turns per-frame stamps into smooth trails and keeps frame-to-frame
 * delta low — nothing in this pattern can appear or vanish in one frame. */
static void p165_tone(float decay)
{
    int i, n = P165_LW * P165_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p165_acc[i];
        int c = (int)(v * 255.0f);
        p165_img[i] = c <= 0 ? 0 : c >= 255 ? 255 : (uint8_t)c;
        p165_acc[i] = v * decay;
    }
}


#define P165_NV 18                      /* vortices in the street  */
#define P165_NP 7200                    /* dye tracers             */
#define P165_X0 96.0f                   /* cylinder centre         */
#define P165_Y0 180.0f
#define P165_RAD 25.0f
#define P165_SPAN 470.0f                /* recirculation length    */

static float p165_px[P165_NP], p165_py[P165_NP];
static uint8_t p165_ph[P165_NP];
static uint32_t p165_rs = 0x1CE5A17Du;
static int p165_ready;

static float p165_rf(void)
{
    p165_rs ^= p165_rs << 13; p165_rs ^= p165_rs >> 17; p165_rs ^= p165_rs << 5;
    return (float)(p165_rs >> 8) * (1.0f / 16777216.0f);
}

static void p165_spawn(int i, int fresh)
{
    p165_px[i] = fresh ? P165_X0 - 40.0f - p165_rf() * 30.0f
                       : P165_X0 - 70.0f + p165_rf() * (P165_SPAN + 60.0f);
    p165_py[i] = 26.0f + p165_rf() * 308.0f;
    p165_ph[i] = (uint8_t)(p165_rf() * 255.0f);
}

void pattern_165(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float s = (float)(seed & 1023) * 0.00613f;
    float vx[P165_NV], vy[P165_NV], vg[P165_NV];
    float u0, sway, spacing, off, dsep;
    int i, k, hb;

    (void)sl;
    if (!p165_ready) {
        p165_rs = 0x1CE5A17Du ^ seed;
        for (i = 0; i < P165_NP; i++) p165_spawn(i, 0);
        p165_ready = 1;
    }
    p165_build_ramp(pal, (int)(t * 6.2f) + (int)(seed & 32767), 92);
    hb = 0;  /* TEMPORAL REVIEW 2.4.0 (F-16x): the discrete hb hue
             * stepper snapped the whole figure one ramp entry at a
             * time; its rate is folded into the ramp base above,
             * which slides continuously. */

    u0      = 1.05f + 0.12f * sinf(t * 0.00041f + s);
    dsep    = 30.0f + 5.0f * sinf(t * 0.00033f + s * 1.3f);
    spacing = P165_SPAN / (float)(P165_NV / 2);
    off     = t * u0 * 0.86f;

    /* the staggered double row. x wraps through the domain; strength fades in
     * and out at the ends so a wrapping vortex never pops. */
    for (k = 0; k < P165_NV; k++) {
        float xk = (float)(k / 2) * spacing + off;
        float e, ph;
        xk -= P165_SPAN * floorf(xk / P165_SPAN);
        vx[k] = P165_X0 + 18.0f + xk;
        ph = (xk / P165_SPAN) * P165_TAU;
        vy[k] = P165_Y0 + ((k & 1) ? dsep : -dsep) * (1.0f + 0.18f * sinf(ph * 2.0f));
        e = xk * (1.0f / 60.0f);
        if (e > 1.0f) e = 1.0f;
        {
            float f = (P165_SPAN - xk) * (1.0f / 90.0f);
            if (f < e) e = f;
            if (e < 0.0f) e = 0.0f;
        }
        e = e * e * (3.0f - 2.0f * e);
        vg[k] = ((k & 1) ? -58.0f : 58.0f) * e;
    }

    sway = 0.9f + 0.4f * sinf(t * 0.00027f);
    for (i = 0; i < P165_NP; i++) {
        float x = p165_px[i], y = p165_py[i];
        float ux = u0, uy = 0.0f, br, g;
        const uint8_t *cp;
        for (k = 0; k < P165_NV; k++) {
            float dx = x - vx[k], dy = y - vy[k];
            float q = vg[k] / (dx * dx + dy * dy + 300.0f);
            ux -= dy * q; uy += dx * q;
        }
        /* the cylinder pushes dye around itself instead of through */
        {
            float dx = x - P165_X0, dy = y - P165_Y0;
            float r2 = dx * dx + dy * dy;
            if (r2 < 5000.0f) {
                float q = 2600.0f / (r2 + 260.0f);
                ux += dx * q * 0.010f; uy += dy * q * 0.010f;
            }
        }
        uy += 0.05f * sinf(y * 0.03f + t * 0.004f) * sway;
        x += ux; y += uy;
        if (x > P165_X0 + P165_SPAN || y < 4.0f || y > 356.0f) {
            p165_spawn(i, 1);
            x = p165_px[i]; y = p165_py[i];
        }
        p165_px[i] = x; p165_py[i] = y;

        br = (x - (P165_X0 - 70.0f)) * (1.0f / 55.0f);
        if (br > 1.0f) br = 1.0f;
        {
            float f = (P165_X0 + P165_SPAN - x) * (1.0f / 70.0f);
            if (f < br) br = f;
            if (br < 0.0f) br = 0.0f;
        }
        cp = p165_ramp[(hb + p165_ph[i] / 3 + (y > P165_Y0 ? 60 : 0)) & 255];
        g  = br * 0.245f;
        p165_splat(x, y, cp[0] * g * (1.0f / 255.0f),
                         cp[1] * g * (1.0f / 255.0f),
                         cp[2] * g * (1.0f / 255.0f));
    }

    /* the cylinder itself: a dim rim, so the cause of the wake is visible */
    for (i = 0; i < 72; i++) {
        float a = (float)i * (P165_TAU / 72.0f);
        const uint8_t *cp = p165_ramp[(hb + 150) & 255];
        float g = 0.28f;
        p165_stroke(P165_X0 + P165_RAD * cosf(a), P165_Y0 + P165_RAD * sinf(a),
                    cp[0] * g * (1.0f / 255.0f), cp[1] * g * (1.0f / 255.0f),
                    cp[2] * g * (1.0f / 255.0f));
    }
    p165_tone(0.930f);
    p165_blit(fb, w, h);
}
