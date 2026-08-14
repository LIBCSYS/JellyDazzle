/* 105 Rule Mandala — Wolfram's elementary automaton wrapped around a circle.
 * A 192-cell ring runs a 1-D elementary cellular automaton with periodic
 * boundaries. Instead of stacking generations downward as the textbooks do,
 * each new generation is laid down at the centre and the older ones are pushed
 * outward, so the automaton's space-time diagram becomes a mandala growing
 * from its own middle: Sierpinski gaskets curl into rosettes, rule 110's
 * gliders spiral, rule 22's triangles fan out into petals. Only left-right
 * symmetric rules are used (90, 150, 18, 22, 126, 182) and the seed is
 * mirrored, which guarantees a perfectly bilateral figure; the rule changes
 * every few hundred frames, and because old rings are never rewritten the
 * change arrives as a new texture growing out of the middle rather than a cut.
 * Radii advance by a fraction of a pixel per frame, so the drift is continuous
 * and slow. Live cells only — the dead ones are black, so it composites. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p105_up;

#define P105_LW 480
#define P105_LH 360
#define P105_NC 256
#define P105_RINGS 104
#define P105_STEP 5                 /* frames per generation */

static float p105_acc[P105_LW * P105_LH * 3];
static uint8_t p105_img[P105_LW * P105_LH * 3];
static int *p105_xm;
static int p105_xm_w;
static uint8_t p105_tone[2048];
static uint8_t p105_ramp[256][3];
static uint8_t p105_hist[P105_RINGS][P105_NC];
static float p105_cs[P105_NC * 4], p105_sn[P105_NC * 4];
static int p105_head;                /* index of the newest generation */
static int p105_gen;
static int p105_last;
static int p105_ready;

static const uint8_t p105_rules[6] = { 90, 150, 18, 22, 126, 182 };

static void p105_init(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 1.0f - expf(-(float)i * (4.4f / 2048.0f));
        p105_tone[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    for (i = 0; i < P105_NC * 4; i++) {
        float a = (float)i * (6.2831853f / (float)(P105_NC * 4));
        p105_cs[i] = cosf(a); p105_sn[i] = sinf(a);
    }
    p105_hist[0][P105_NC / 2] = 1;      /* one live cell, mirrored by symmetry */
    p105_last = -1000000;
    p105_ready = 1;
}

static void p105_build_ramp(const uint32_t *pal, int base)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * 128) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p105_ramp[i][0] = p105_ramp[i-1][0];
                     p105_ramp[i][1] = p105_ramp[i-1][1];
                     p105_ramp[i][2] = p105_ramp[i-1][2]; }
            else   { p105_ramp[i][0] = p105_ramp[i][1] = p105_ramp[i][2] = 210; }
            continue;
        }
        p105_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p105_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p105_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static void p105_dot(float x, float y, const uint8_t *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, *p;
    float r = c[0] * wgt, g = c[1] * wgt, b = c[2] * wgt;
    if ((unsigned)xi >= P105_LW - 1 || (unsigned)yi >= P105_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p105_acc + (yi * P105_LW + xi) * 3;
    {
        float w00 = (1.0f - fx) * (1.0f - fy), w10 = fx * (1.0f - fy);
        float w01 = (1.0f - fx) * fy, w11 = fx * fy;
        p[0] += r * w00; p[1] += g * w00; p[2] += b * w00;
        p[3] += r * w10; p[4] += g * w10; p[5] += b * w10;
        p += P105_LW * 3;
        p[0] += r * w01; p[1] += g * w01; p[2] += b * w01;
        p[3] += r * w11; p[4] += g * w11; p[5] += b * w11;
    }
}

static void p105_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p105_xm_w != w) {
        free(p105_xm);
        p105_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p105_xm[x] = (int)(((long long)x * (P105_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p105_xm_w = w;
    }
    jd_up_blit(&p105_up, fb, w, h, p105_img, P105_LW, P105_LH);
}

void pattern_105(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float frac, dr, r0, spin, cx, cy;
    int i, g, n3 = P105_LW * P105_LH * 3, hbase;
    (void)sl;

    if (!p105_ready) p105_init();
    hbase = (int)(t * 1.2f) + (int)(seed & 32767);
    p105_build_ramp(pal, hbase);

    /* --- advance the automaton on its own clock --- */
    if (frame < p105_last || frame - p105_last >= P105_STEP) {
        const uint8_t *src = p105_hist[p105_head];
        uint8_t *dst;
        int rule = p105_rules[(p105_gen / 96) % 6];
        p105_head = (p105_head + 1) % P105_RINGS;
        dst = p105_hist[p105_head];
        for (i = 0; i < P105_NC; i++) {
            int l = src[(i + P105_NC - 1) % P105_NC];
            int c = src[i];
            int r = src[(i + 1) % P105_NC];
            dst[i] = (uint8_t)((rule >> ((l << 2) | (c << 1) | r)) & 1);
        }
        if (p105_gen % 96 == 95) {           /* re-seed when the rule changes */
            memset(dst, 0, P105_NC);
            dst[P105_NC / 2] = 1;
            dst[(P105_NC / 2 + P105_NC / 3) % P105_NC] = 1;
            dst[(P105_NC / 2 - P105_NC / 3 + P105_NC) % P105_NC] = 1;
        }
        p105_gen++;
        p105_last = frame;
    }
    frac = (float)(frame - p105_last) * (1.0f / (float)P105_STEP);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    dr = 1.86f;
    r0 = 5.0f;
    spin = t * 0.00047f + (float)(seed & 1023) * 0.0061f;
    cx = P105_LW * 0.5f; cy = P105_LH * 0.5f;

    memset(p105_acc, 0, sizeof p105_acc);
    for (g = 0; g < P105_RINGS; g++) {
        const uint8_t *row = p105_hist[(p105_head - g + P105_RINGS * 2) % P105_RINGS];
        float r = r0 + ((float)g + frac) * dr;
        int hue = (hbase / 20 + g * 3) & 255;
        const uint8_t *cp = p105_ramp[hue];
        float bri = (g < 3) ? 0.95f : 0.62f;
        int sub, spq;
        if (r > 205.0f) break;
        /* angular samples per cell, so the arc stays solid as r grows */
        sub = (int)(r * (6.2831853f / (float)P105_NC) * 1.25f) + 1;
        if (sub > 8) sub = 8;
        spq = (int)(spin * (float)(P105_NC * 4) * 0.1591549f);
        for (i = 0; i < P105_NC; i++) {
            int k;
            if (!row[i]) continue;
            for (k = 0; k < sub; k++) {
                int q = ((i * 4) + (k * 4) / sub + spq) & (P105_NC * 4 - 1);
                float px = cx + r * p105_cs[q] * 1.06f;
                float py = cy + r * p105_sn[q] * 0.82f;
                p105_dot(px, py, cp, bri * 0.30f / (float)sub);
                p105_dot(px + p105_cs[q] * 0.9f, py + p105_sn[q] * 0.7f,
                         cp, bri * 0.22f / (float)sub);
            }
        }
    }

    for (i = 0; i < n3; i++) {
        int ti = (int)(p105_acc[i] * 22.0f);
        if (ti > 2047) ti = 2047;
        p105_img[i] = p105_tone[ti];
    }
    p105_blit(fb, w, h);
}
