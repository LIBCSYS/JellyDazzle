/* 170 Gyroid Veil — a slice through a triply-periodic minimal surface.
 * The gyroid is the zero set of sin x cos y + sin y cos z + sin z cos x. Hold z
 * and you get a labyrinth of curves in the plane; let z drift and the labyrinth
 * continuously splits, reconnects and re-knits itself, because you are watching
 * a genuine 3-D surface pass through the frame. Two contour levels are drawn as
 * filaments, so the picture is line work rather than fill, and the sampling
 * plane slowly rotates and zooms. All of it is a 1024-entry sine LUT and three
 * multiplies per pixel — no transcendentals in the loop. Sparse on black. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p170_up;

#define P170_LW 480
#define P170_LH 360
#define P170_TAU 6.28318530717958647692f

static uint8_t p170_img[P170_LW * P170_LH * 3];
static int    *p170_xm;
static int     p170_xm_w;
static uint8_t p170_ramp[256][3];

/* 256-entry hue ramp lifted from the live palette. Palette schemes contain
 * near-black stretches; a too-dark sample inherits its predecessor so the
 * figure never gets punched full of dark holes. */
static void p170_build_ramp(const uint32_t *pal, int base, int span)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * span) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p170_ramp[i][0] = p170_ramp[i-1][0];
                     p170_ramp[i][1] = p170_ramp[i-1][1];
                     p170_ramp[i][2] = p170_ramp[i-1][2]; }
            else   { p170_ramp[i][0] = p170_ramp[i][1] = p170_ramp[i][2] = 255; }
            continue;
        }
        p170_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p170_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p170_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

/* bilinear upscale of the low-res image onto the real framebuffer */
static void p170_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p170_xm_w != w) {
        free(p170_xm);
        p170_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p170_xm[x] = (int)(((long long)x * (P170_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p170_xm_w = w;
    }
    jd_up_blit(&p170_up, fb, w, h, p170_img, P170_LW, P170_LH);
}


static float   p170_sin[1024];
static uint8_t p170_core[1024], p170_halo[1024];
static uint8_t p170_vig[P170_LW * P170_LH];
static int     p170_ready;

static void p170_build(void)
{
    int i, x, y;
    for (i = 0; i < 1024; i++)
        p170_sin[i] = sinf((float)i * (P170_TAU / 1024.0f));
    for (i = 0; i < 1024; i++) {
        float a = (float)i * (2.2f / 1024.0f);
        p170_core[i] = (uint8_t)(255.0f / (1.0f + 78.0f * a * a));
        p170_halo[i] = (uint8_t)(255.0f / (1.0f + 5.0f * a * a));
    }
    for (y = 0; y < P170_LH; y++) {
        float dy = ((float)y - 180.0f) / 180.0f;
        for (x = 0; x < P170_LW; x++) {
            float dx = ((float)x - 240.0f) / 240.0f;
            float r = sqrtf(dx * dx + dy * dy * 0.92f);
            float e = (1.16f - r) * 2.4f;
            if (e < 0.0f) e = 0.0f; if (e > 1.0f) e = 1.0f;
            p170_vig[y * P170_LW + x] = (uint8_t)(e * e * (3.0f - 2.0f * e) * 255.0f);
        }
    }
    p170_ready = 1;
}

void pattern_170(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float s = (float)(seed & 1023) * 0.00613f;
    float zz, cz, sz, scale, rot, lev;
    int a_fix, b_fix, ox, oy, x, y, hb;
    (void)sl;

    if (!p170_ready) p170_build();
    p170_build_ramp(pal, (int)(t * 1.5f) + (int)(seed & 32767), 80);
    hb = (int)(t * 0.05f);

    zz    = t * 0.0016f + s;                    /* the slice depth drifts     */
    cz    = cosf(zz); sz = sinf(zz);
    scale = 0.0455f * (1.0f + 0.14f * sinf(t * 0.00029f + s * 1.3f));
    rot   = t * 0.00021f + s * 0.5f;
    lev   = 0.62f + 0.16f * sinf(t * 0.00037f);

    a_fix = (int)(scale * cosf(rot) * 10430.38f);
    b_fix = (int)(scale * sinf(rot) * 10430.38f);
    ox    = (int)(t * 3.1f);
    oy    = (int)(t * 1.9f);

    for (y = 0; y < P170_LH; y++) {
        int px = ox + y * b_fix, py = oy + y * a_fix;
        uint8_t *row = p170_img + y * P170_LW * 3;
        const uint8_t *vg = p170_vig + y * P170_LW;
        for (x = 0; x < P170_LW; x++, px += a_fix, py -= b_fix) {
            int ix = (px >> 6) & 1023, iy = (py >> 6) & 1023;
            float sxv = p170_sin[ix], cxv = p170_sin[(ix + 256) & 1023];
            float syv = p170_sin[iy], cyv = p170_sin[(iy + 256) & 1023];
            float f = sxv * cyv + syv * cz + sz * cxv;
            float d0 = fabsf(f);
            float d1 = fabsf(d0 - lev);
            int i0 = (int)(d0 * 465.0f), i1 = (int)(d1 * 465.0f);
            int lum, hue, o = x * 3;
            const uint8_t *cp;
            if (i0 > 1023) i0 = 1023;
            if (i1 > 1023) i1 = 1023;
            lum = p170_core[i0] + ((p170_core[i1] * 118) >> 8)
                                + ((p170_halo[i0] * 16) >> 8);
            if (lum > 255) lum = 255;
            lum = (lum * vg[x]) >> 8;
            if (lum < 3) { row[o] = row[o+1] = row[o+2] = 0; continue; }
            hue = (hb + (int)(f * 52.0f)) & 255;
            cp  = p170_ramp[hue];
            row[o + 0] = (uint8_t)((cp[0] * lum) >> 8);
            row[o + 1] = (uint8_t)((cp[1] * lum) >> 8);
            row[o + 2] = (uint8_t)((cp[2] * lum) >> 8);
        }
    }
    p170_blit(fb, w, h);
}
