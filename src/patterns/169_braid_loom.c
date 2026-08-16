/* 169 Braid Loom — seven strands plaiting over and under each other.
 * Every strand is a sine of the same wavelength at its own phase; the COSINE of
 * that phase is its depth, so the strands really do pass in front of and behind
 * one another. Each column of the image is painted back to front, and every
 * strand lays down a dark rim before its own shading, which is what produces
 * the clean over-under crossings of a real braid rather than a pile of curves.
 * The whole band snakes slowly across the frame. Painter's algorithm, no
 * accumulator: strands on black, so this reads as a sparse ribbon overlay. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p169_up;

#define P169_LW 480
#define P169_LH 360
#define P169_TAU 6.28318530717958647692f

static uint8_t p169_img[P169_LW * P169_LH * 3];
static int    *p169_xm;
static int     p169_xm_w;
static uint8_t p169_ramp[256][3];

/* 256-entry hue ramp lifted from the live palette. Palette schemes contain
 * near-black stretches; a too-dark sample inherits its predecessor so the
 * figure never gets punched full of dark holes. */
static void p169_build_ramp(const uint32_t *pal, int base, int span)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * span) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p169_ramp[i][0] = p169_ramp[i-1][0];
                     p169_ramp[i][1] = p169_ramp[i-1][1];
                     p169_ramp[i][2] = p169_ramp[i-1][2]; }
            else   { p169_ramp[i][0] = p169_ramp[i][1] = p169_ramp[i][2] = 255; }
            continue;
        }
        p169_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p169_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p169_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

/* bilinear upscale of the low-res image onto the real framebuffer */
static void p169_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p169_xm_w != w) {
        free(p169_xm);
        p169_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p169_xm[x] = (int)(((long long)x * (P169_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p169_xm_w = w;
    }
    jd_up_blit(&p169_up, fb, w, h, p169_img, P169_LW, P169_LH);
}


#define P169_S 7

void pattern_169(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float s = (float)(seed & 1023) * 0.00613f;
    float k, om, amp, hw, rim, spec;
    int x, i, j, hb;
    (void)sl;

    p169_build_ramp(pal, (int)(t * 3.5f) + (int)(seed & 32767), 66);
    hb = 0;  /* TEMPORAL REVIEW 2.4.0 (F-16x): the discrete hb hue
             * stepper snapped the whole figure one ramp entry at a
             * time; its rate is folded into the ramp base above,
             * which slides continuously. */
    memset(p169_img, 0, sizeof p169_img);

    k    = 0.0255f + 0.0035f * sinf(t * 0.00027f + s);
    om   = 0.0125f;
    amp  = 40.0f + 7.0f * sinf(t * 0.00033f + s * 1.7f);
    hw   = 7.4f;
    rim  = 2.6f;
    spec = t * 0.9f;

    for (x = 0; x < P169_LW; x++) {
        float fx = (float)x;
        float cy = 180.0f + 44.0f * sinf(fx * 0.0072f + t * 0.0031f + s)
                          + 12.0f * sinf(fx * 0.019f - t * 0.0017f);
        float yy[P169_S], zz[P169_S];
        int   ord[P169_S];
        for (i = 0; i < P169_S; i++) {
            float p = k * fx + om * t + (float)i * (P169_TAU / (float)P169_S);
            yy[i] = cy + amp * sinf(p);
            zz[i] = cosf(p);
            ord[i] = i;
        }
        for (i = 1; i < P169_S; i++) {           /* insertion sort, far first */
            int v = ord[i];
            for (j = i; j > 0 && zz[ord[j-1]] > zz[v]; j--) ord[j] = ord[j-1];
            ord[j] = v;
        }
        for (i = 0; i < P169_S; i++) {
            int id = ord[i];
            float yc = yy[id], z = zz[id];
            float depth = 0.52f + 0.48f * (0.5f + 0.5f * z);
            const uint8_t *cp = p169_ramp[(hb + id * 34) & 255];
            float hl = 0.5f + 0.5f * sinf((fx - spec) * 0.021f + (float)id * 0.9f);
            int ya = (int)(yc - hw - rim), yb = (int)(yc + hw + rim), y;
            if (ya < 0) ya = 0;
            if (yb > P169_LH - 1) yb = P169_LH - 1;
            for (y = ya; y <= yb; y++) {
                float dy = (float)y - yc;
                float a = fabsf(dy) / hw;
                uint8_t *o = p169_img + (y * P169_LW + x) * 3;
                int lum;
                if (a >= 1.0f) { o[0] = o[1] = o[2] = 0; continue; }
                a = 1.0f - a * a;                       /* tube cross-section  */
                lum = (int)((0.30f + 0.70f * a) * depth *
                            (0.72f + 0.55f * hl * a * a) * 255.0f);
                if (lum > 255) lum = 255;
                o[0] = (uint8_t)((cp[0] * lum) >> 8);
                o[1] = (uint8_t)((cp[1] * lum) >> 8);
                o[2] = (uint8_t)((cp[2] * lum) >> 8);
            }
        }
    }
    p169_blit(fb, w, h);
}
