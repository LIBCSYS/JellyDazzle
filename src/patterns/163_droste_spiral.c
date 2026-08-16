/* 163 Droste Spiral — a conformal self-similar zoom that never ends.
 * The frame is mapped to log-polar coordinates (ln r, theta) and a lattice of
 * glowing beads is laid out on an INTEGER lattice in that plane, which is what
 * makes the picture exactly seamless: going once around the centre, or one
 * scale step outward, lands on the same lattice point. The result is Escher's
 * Droste effect — a spiral of beads that shrinks into the middle forever while
 * the whole field slowly rotates and zooms. A bright, full-field ground layer.
 * Everything is integer LUT work after a one-off log/atan2 per pixel. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p163_up;

#define P163_LW 480
#define P163_LH 360
#define P163_TAU 6.28318530717958647692f

static uint8_t p163_img[P163_LW * P163_LH * 3];
static int    *p163_xm;
static int     p163_xm_w;
static uint8_t p163_ramp[256][3];

/* 256-entry hue ramp lifted from the live palette. Palette schemes contain
 * near-black stretches; a too-dark sample inherits its predecessor so the
 * figure never gets punched full of dark holes. */
static void p163_build_ramp(const uint32_t *pal, int base, int span)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * span) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p163_ramp[i][0] = p163_ramp[i-1][0];
                     p163_ramp[i][1] = p163_ramp[i-1][1];
                     p163_ramp[i][2] = p163_ramp[i-1][2]; }
            else   { p163_ramp[i][0] = p163_ramp[i][1] = p163_ramp[i][2] = 255; }
            continue;
        }
        p163_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p163_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p163_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

/* bilinear upscale of the low-res image onto the real framebuffer */
static void p163_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p163_xm_w != w) {
        free(p163_xm);
        p163_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p163_xm[x] = (int)(((long long)x * (P163_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p163_xm_w = w;
    }
    jd_up_blit(&p163_up, fb, w, h, p163_img, P163_LW, P163_LH);
}


static uint16_t p163_afix[P163_LW * P163_LH];   /* theta / 2pi * 65536      */
static int16_t  p163_bfix[P163_LW * P163_LH];   /* ln(r) * 2048             */
static uint8_t  p163_vig[P163_LW * P163_LH];    /* edge vignette            */
static uint8_t  p163_bead[512];                 /* bead falloff by d^2      */
static uint8_t  p163_web[512];                  /* thin cell-edge filament  */
static int      p163_ready;

static void p163_build_geom(void)
{
    int x, y, i;
    float inv = 1.0f / (0.46f * (float)P163_LH);
    for (y = 0; y < P163_LH; y++) {
        float dy = (float)y - (float)P163_LH * 0.5f + 0.5f;
        for (x = 0; x < P163_LW; x++) {
            float dx = (float)x - (float)P163_LW * 0.5f + 0.5f;
            float rr = sqrtf(dx * dx + dy * dy) * inv;
            float aa, lb, e;
            i = y * P163_LW + x;
            if (rr < 0.0022f) rr = 0.0022f;
            aa = atan2f(dy, dx) * (1.0f / P163_TAU);
            aa -= floorf(aa);
            p163_afix[i] = (uint16_t)(aa * 65535.0f);
            lb = logf(rr) * 2048.0f;
            p163_bfix[i] = (int16_t)(lb < -32000.0f ? -32000.0f : lb);
            e = (1.32f - rr) * 2.6f;                 /* soft frame vignette */
            if (e < 0.0f) e = 0.0f; if (e > 1.0f) e = 1.0f;
            p163_vig[i] = (uint8_t)(e * e * (3.0f - 2.0f * e) * 255.0f);
        }
    }
    for (i = 0; i < 512; i++) {
        float d2 = (float)i * (1.0f / 511.0f);        /* normalised r^2 in cell */
        p163_bead[i] = (uint8_t)(255.0f / (1.0f + 120.0f * d2 * d2 * d2));
        p163_web[i]  = (uint8_t)(255.0f / (1.0f + 26.0f * d2 * d2));
    }
    p163_ready = 1;
}

void pattern_163(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    int i, n, ua, ub, va, vb, uoff, voff, spin, hb, arms;
    (void)sl;

    if (!p163_ready) p163_build_geom();
    p163_build_ramp(pal, (int)(t * 7.6f) + (int)(seed & 32767), 104);

    /* the lattice: u = arms.theta + b, v = b, both integer-periodic, so the
     * mapping is seamless across theta = 0 and across one scale step */
    arms = 9;                    /* cells are square in the log-polar plane
                                  * when arms ~ 2.pi/ln(2); keep it constant so
                                  * a new segment seed can never jump the
                                  * lattice geometry */
    ua = arms; ub = 1; va = 0; vb = 1;
    uoff = (int)(t * 1.70f);
    voff = (int)(t * 1.05f);
    spin = (int)(t * 11.0f);                       /* slow whole-field turn   */
    hb   = 0;  /* TEMPORAL REVIEW 2.4.0 (F-16x, docs/review/
                     * 04_pattern_temporal.md): hb stepped one whole ramp
                     * entry every ~17 frames — a whole-frame hue snap.
                     * Its average rate is folded into the ramp base above
                     * (which already slides continuously per frame). */
    n    = P163_LW * P163_LH;

    for (i = 0; i < n; i++) {
        int a = (int)((p163_afix[i] + spin) & 0xFFFF);
        int b = (int)p163_bfix[i];
        int bs = (b * 738) >> 10;                  /* b / ln(2) in 1/1024ths */
        int u = ua * (a >> 6) + ub * bs + uoff;
        int v = va * (a >> 6) + vb * bs + voff;
        int du = (u & 1023) - 512, dv = (v & 1023) - 512;
        int d2 = ((du * du + dv * dv) >> 10);
        int lum, hue, o = i * 3;
        const uint8_t *cp;
        if (d2 > 511) d2 = 511;
        lum = p163_bead[d2] + ((p163_web[d2] * 52) >> 8);
        if (lum > 255) lum = 255;
        lum = (lum * p163_vig[i]) >> 8;
        hue = (hb + ((v >> 4) & 63) + ((u >> 6) & 31)) & 255;
        cp  = p163_ramp[hue];
        p163_img[o + 0] = (uint8_t)((cp[0] * lum) >> 8);
        p163_img[o + 1] = (uint8_t)((cp[1] * lum) >> 8);
        p163_img[o + 2] = (uint8_t)((cp[2] * lum) >> 8);
    }
    p163_blit(fb, w, h);
}
