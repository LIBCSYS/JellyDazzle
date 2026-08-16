/* 164 Kolam Loom — the single unbroken stroke of a South-Indian pulli kolam.
 * A ball bouncing at 45 degrees inside a rectangle traces one closed loop that
 * threads around every dot of a lattice; in closed form that is a Lissajous
 * figure built from TRIANGLE waves instead of sines. Rounding the corners of
 * those waves gives the soft rice-flour curve of a real kolam. The two wave
 * counts drift continuously, so the loop keeps re-weaving itself into a new
 * lattice without ever being redrawn, and a bead of light runs along the
 * stroke the way the hand runs along the powder. Dots and stroke on black.
 * The plate is deliberately never cleared on an sl discontinuity: it refreshes
 * itself within ~30 frames, so clearing it would only add a hard cut. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p164_up;

#define P164_LW 480
#define P164_LH 360
#define P164_TAU 6.28318530717958647692f

static uint8_t p164_img[P164_LW * P164_LH * 3];
static int    *p164_xm;
static int     p164_xm_w;
static uint8_t p164_ramp[256][3];

/* 256-entry hue ramp lifted from the live palette. Palette schemes contain
 * near-black stretches; a too-dark sample inherits its predecessor so the
 * figure never gets punched full of dark holes. */
static void p164_build_ramp(const uint32_t *pal, int base, int span)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * span) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p164_ramp[i][0] = p164_ramp[i-1][0];
                     p164_ramp[i][1] = p164_ramp[i-1][1];
                     p164_ramp[i][2] = p164_ramp[i-1][2]; }
            else   { p164_ramp[i][0] = p164_ramp[i][1] = p164_ramp[i][2] = 255; }
            continue;
        }
        p164_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p164_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p164_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

/* bilinear upscale of the low-res image onto the real framebuffer */
static void p164_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p164_xm_w != w) {
        free(p164_xm);
        p164_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p164_xm[x] = (int)(((long long)x * (P164_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p164_xm_w = w;
    }
    jd_up_blit(&p164_up, fb, w, h, p164_img, P164_LW, P164_LH);
}

static float p164_acc[P164_LW * P164_LH * 3];

/* bilinear additive splat */
static void p164_splat(float x, float y, float r, float g, float b)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w00, w01, w10, w11;
    float *q;
    if (xi < 0 || yi < 0 || xi >= P164_LW - 1 || yi >= P164_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy); w01 = fx * (1.0f - fy);
    w10 = (1.0f - fx) * fy;          w11 = fx * fy;
    q = p164_acc + (yi * P164_LW + xi) * 3;
    q[0] += r * w00; q[1] += g * w00; q[2] += b * w00;
    q[3] += r * w01; q[4] += g * w01; q[5] += b * w01;
    q += P164_LW * 3;
    q[0] += r * w10; q[1] += g * w10; q[2] += b * w10;
    q[3] += r * w11; q[4] += g * w11; q[5] += b * w11;
}

/* soft 5-tap stroke: bilinear core plus a dim cross, so a one-pixel figure
 * survives the upscale as a glowing filament instead of a hairline */
static void p164_stroke(float x, float y, float r, float g, float b)
{
    p164_splat(x, y, r, g, b);
    r *= 0.30f; g *= 0.30f; b *= 0.30f;
    p164_splat(x - 1.6f, y, r, g, b);
    p164_splat(x + 1.6f, y, r, g, b);
    p164_splat(x, y - 1.6f, r, g, b);
    p164_splat(x, y + 1.6f, r, g, b);
}

/* one pass: tone-map the accumulator into the image and decay it. The decay is
 * what turns per-frame stamps into smooth trails and keeps frame-to-frame
 * delta low — nothing in this pattern can appear or vanish in one frame. */
static void p164_tone(float decay)
{
    int i, n = P164_LW * P164_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p164_acc[i];
        int c = (int)(v * 255.0f);
        p164_img[i] = c <= 0 ? 0 : c >= 255 ? 255 : (uint8_t)c;
        p164_acc[i] = v * decay;
    }
}


#define P164_M 6000                       /* samples along the stroke */

static float p164_tri[1024];

static void p164_build_tri(float k)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float u = (float)i * (1.0f / 1024.0f);
        float sn = sinf(u * P164_TAU);
        float tw = asinf(sn) * (2.0f / 3.14159265f);
        p164_tri[i] = (1.0f - k) * sn + k * tw;
    }
}

void pattern_164(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float s = (float)(seed & 1023) * 0.00613f;
    float a, b, ph, rot, cs, sn, rx, ry, comet, k;
    int i, hb;


    (void)sl;
    k = 0.90f + 0.09f * sinf(t * 0.00037f + s);
    p164_build_tri(k);
    p164_build_ramp(pal, (int)(t * 8.1f) + (int)(seed & 32767), 88);
    hb = 0;  /* TEMPORAL REVIEW 2.4.0 (F-16x): the discrete hb hue
             * stepper snapped the whole figure one ramp entry at a
             * time; its rate is folded into the ramp base above,
             * which slides continuously. */

    a   = 4.0f + 2.6f * (0.5f + 0.5f * sinf(t * 0.00023f + s));
    b   = 5.0f + 3.0f * (0.5f + 0.5f * sinf(t * 0.00017f + s * 1.7f + 2.0f));
    ph  = t * 0.00042f;
    rot = 0.20f * sinf(t * 0.00019f + 0.6f);
    cs  = cosf(rot); sn = sinf(rot);
    rx  = 196.0f * (1.0f + 0.035f * sinf(t * 0.00051f));
    ry  = 146.0f * (1.0f + 0.035f * cosf(t * 0.00047f));
    comet = t * 0.00055f;

    /* the pulli: a faint dot lattice the stroke weaves around */
    for (i = 0; i < 63; i++) {
        int gx = i % 9, gy = i / 9;
        float ux = ((float)gx - 4.0f) * (rx / 4.6f);
        float uy = ((float)gy - 3.0f) * (ry / 3.6f);
        float px = 240.0f + ux * cs - uy * sn;
        float py = 180.0f + ux * sn + uy * cs;
        const uint8_t *cp = p164_ramp[(hb + 128) & 255];
        float g = 0.22f;
        p164_stroke(px, py, cp[0] * g * (1.0f / 255.0f),
                            cp[1] * g * (1.0f / 255.0f),
                            cp[2] * g * (1.0f / 255.0f));
    }

    /* the stroke itself */
    for (i = 0; i < P164_M; i++) {
        float u = (float)i * (1.0f / (float)P164_M);
        float ux = rx * p164_tri[((int)(u * a * 1024.0f)) & 1023];
        float uy = ry * p164_tri[((int)((u * b + ph) * 1024.0f)) & 1023];
        float px = 240.0f + ux * cs - uy * sn;
        float py = 180.0f + ux * sn + uy * cs;
        float d  = u - comet;
        float band, glow, g;
        const uint8_t *cp;
        d -= floorf(d);
        band = 0.5f + 0.5f * cosf(d * P164_TAU);
        band = band * band; band = band * band; band = band * band;
        glow = 0.30f + 0.95f * band;
        cp = p164_ramp[(hb + (int)(u * 210.0f)) & 255];
        g  = glow * 0.185f;
        p164_stroke(px, py, cp[0] * g * (1.0f / 255.0f),
                            cp[1] * g * (1.0f / 255.0f),
                            cp[2] * g * (1.0f / 255.0f));
    }
    p164_tone(0.80f);
    p164_blit(fb, w, h);
}
