/* 113 Sandpile Mandala — the abelian sandpile, growing in real time.
 * Grains are dropped on the centre cell of a 385x385 lattice; any cell holding
 * four or more topples, giving one grain to each neighbour, and the cascade is
 * run to completion through a toppling queue (order does not matter — that
 * is the theorem the model is named for), so the pile is fully relaxed in
 * every rendered frame. What emerges is not
 * noise but the famous crystalline fractal: nested triangular domains of
 * height 0,1,2,3 with sharp linear boundaries, growing as sqrt(grains). The
 * lattice is drawn one cell per canvas pixel, cells mid-avalanche
 * are drawn hot, and a twenty-frame temporal average turns each cell's change
 * of height into a soft melt rather than a flicker. Mid-density, sharply structured: a ground layer.
 */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
static jd_up p113_up;

#define P113_G 385
#define P113_C (P113_G / 2)
#define P113_LW 512
#define P113_LH 384

static unsigned char p113_h[P113_G * P113_G];
static int p113_init, p113_x0, p113_x1, p113_y0, p113_y1;
static float p113_zoom = 12.0f;
static float p113_sm[P113_LW * P113_LH * 3];
static unsigned char p113_img[P113_LW * P113_LH * 3];
static float p113_ramp[256][3];
static int p113_prime;

static void p113_ramp_build(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i * 128) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 8.0f) mx = 8.0f;
        p113_ramp[i][0] = r / mx; p113_ramp[i][1] = g / mx; p113_ramp[i][2] = b / mx;
    }
}

/* toppling queue: relaxation costs one visit per actual topple, not one sweep
 * per cell, which is what lets the pile stay fully relaxed every frame */
#define P113_QCAP (1 << 18)
static int p113_q[P113_QCAP];
static unsigned char p113_inq[P113_G * P113_G];
static int p113_qh, p113_qt;

static void p113_push(int idx)
{
    int nt;
    if (p113_inq[idx]) return;
    nt = (p113_qt + 1) & (P113_QCAP - 1);
    if (nt == p113_qh) return;                 /* full: caught on a later pass */
    p113_q[p113_qt] = idx;
    p113_qt = nt;
    p113_inq[idx] = 1;
}

static void p113_relax(int budget)
{
    while (p113_qh != p113_qt && budget-- > 0) {
        int idx = p113_q[p113_qh];
        int x = idx % P113_G, y = idx / P113_G;
        p113_qh = (p113_qh + 1) & (P113_QCAP - 1);
        p113_inq[idx] = 0;
        while (p113_h[idx] >= 4) {
            p113_h[idx] = (unsigned char)(p113_h[idx] - 4);
            if (x > 0)            { p113_h[idx - 1]++; if (p113_h[idx - 1] >= 4) p113_push(idx - 1); }
            if (x < P113_G - 1)   { p113_h[idx + 1]++; if (p113_h[idx + 1] >= 4) p113_push(idx + 1); }
            if (y > 0)            { p113_h[idx - P113_G]++; if (p113_h[idx - P113_G] >= 4) p113_push(idx - P113_G); }
            if (y < P113_G - 1)   { p113_h[idx + P113_G]++; if (p113_h[idx + P113_G] >= 4) p113_push(idx + P113_G); }
            if (x < p113_x0 && x > 1) p113_x0 = x - 1;
            if (x > p113_x1 && x < P113_G - 2) p113_x1 = x + 1;
            if (y < p113_y0 && y > 1) p113_y0 = y - 1;
            if (y > p113_y1 && y < P113_G - 2) p113_y1 = y + 1;
        }
    }
}

void pattern_113(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sp = (float)(seed & 1023) * 0.006136f;
    float cols[5][3];
    float target, z, cx, cy;
    int i, x, y, k, hbase;
    (void)sl;

    p113_ramp_build(pal);
    if (!p113_init) {
        memset(p113_h, 0, sizeof p113_h);
        p113_x0 = p113_y0 = P113_C - 1;
        p113_x1 = p113_y1 = P113_C + 1;
        p113_h[(size_t)P113_C * P113_G + P113_C] = 3;
        p113_init = 1;
    }

    /* feed and relax */
    {
        int room = (p113_x1 - p113_x0 < P113_G - 14) &&
                   (p113_y1 - p113_y0 < P113_G - 14);
        int ctr = P113_C * P113_G + P113_C;
        (void)room;
        if (p113_qh == p113_qt) {
            p113_h[ctr] = (unsigned char)(p113_h[ctr] + 190);
            p113_push(ctr);
        }
        p113_relax(300000);
        (void)k;
    }

    hbase = (int)(t * 0.031f + sp * 30.0f);
    for (i = 0; i < 5; i++) {
        const float *cc = p113_ramp[(hbase + i * 47) & 255];
        float dim = i == 0 ? 0.13f : 0.38f + 0.21f * (float)i;
        if (i == 4) dim = 1.35f;                      /* mid-avalanche cells */
        cols[i][0] = cc[0] * dim; cols[i][1] = cc[1] * dim; cols[i][2] = cc[2] * dim;
    }

    /* one lattice cell per canvas pixel: the pile grows into the frame instead
     * of the frame zooming to follow it, which keeps every moving edge one
     * pixel wide and the motion budget tiny */
    target = (float)P113_LH / (float)P113_G;
    p113_zoom = target;
    z  = 1.0f / p113_zoom;                     /* lattice cells per canvas px */
    cx = (float)P113_LW * 0.5f;
    cy = (float)P113_LH * 0.5f;

    for (y = 0; y < P113_LH; y++) {
        float ly = (float)P113_C + ((float)y - cy) * z;
        int yi = (int)(ly + 0.5f);
        unsigned char *dst = p113_img + (size_t)y * P113_LW * 3;
        for (x = 0; x < P113_LW; x++) {
            float lx = (float)P113_C + ((float)x - cx) * z;
            int xi = (int)(lx + 0.5f);
            const float *c;
            if ((unsigned)xi >= P113_G || (unsigned)yi >= P113_G) c = cols[0];
            else {
                int v = p113_h[(size_t)yi * P113_G + xi];
                c = cols[v > 3 ? 4 : v];
            }
            dst[x * 3 + 0] = (unsigned char)(c[0] * 255.0f > 255.0f ? 255
                                             : c[0] * 255.0f);
            dst[x * 3 + 1] = (unsigned char)(c[1] * 255.0f > 255.0f ? 255
                                             : c[1] * 255.0f);
            dst[x * 3 + 2] = (unsigned char)(c[2] * 255.0f > 255.0f ? 255
                                             : c[2] * 255.0f);
        }
    }

    /* temporal average: the toppling front would otherwise sparkle */
    {
        int n = P113_LW * P113_LH * 3;
        if (!p113_prime) {
            for (i = 0; i < n; i++) p113_sm[i] = (float)p113_img[i];
            p113_prime = 1;
        } else {
            for (i = 0; i < n; i++)
                p113_sm[i] += ((float)p113_img[i] - p113_sm[i]) * 0.055f;
        }
        for (i = 0; i < n; i++) p113_img[i] = (unsigned char)p113_sm[i];
    }

    jd_up_blit(&p113_up, fb, w, h, p113_img, P113_LW, P113_LH);
}
