/* 114 Khatim Weave — eight-fold Islamic strapwork, woven over and under.
 * Two square lattices, one offset half a cell from the other, each carry an
 * octagon outline: the octagonal metric max(|u|,|v|,(|u|+|v|)/sqrt2) makes the
 * outline exactly, with no polygon list. Where the two families of octagons
 * cross they cut the classic khatim star-and-cross figure. Each strap is drawn
 * as a bright core inside a black edge, and at every crossing the lattice
 * parity decides which family is on top — so the bands genuinely interlace
 * rather than merely overlapping. A light runs along the straps (a phase term
 * in the along-band coordinate), the whole panel rotates about a degree every
 * fifteen seconds and breathes in scale. Line art on near-black: an overlay
 * that also reads as a lattice ground.
 */
#include "../jellydazzle.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#define P114_R2 0.70710678f

static float *p114_dx, *p114_dy;
static int p114_w = -1, p114_h = -1;
static float p114_ramp[256][3];
static float p114_sin[1024];
static int p114_tab;

static void p114_ramp_build(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i * 128) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 8.0f) mx = 8.0f;
        p114_ramp[i][0] = r / mx; p114_ramp[i][1] = g / mx; p114_ramp[i][2] = b / mx;
    }
}

static void p114_grid(int w, int h)
{
    int x, y;
    float cx = (float)w * 0.5f, cy = (float)h * 0.5f;
    free(p114_dx); free(p114_dy);
    p114_dx = (float *)malloc(sizeof(float) * (size_t)w);
    p114_dy = (float *)malloc(sizeof(float) * (size_t)h);
    for (x = 0; x < w; x++) p114_dx[x] = (float)x + 0.5f - cx;
    for (y = 0; y < h; y++) p114_dy[y] = (float)y + 0.5f - cy;
    p114_w = w; p114_h = h;
}

void pattern_114(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sp = (float)(seed & 1023) * 0.006136f;
    float ca, sa, cell, inv, rad, kcore, kedge, wcore, wedge, glow;
    const float *inkA, *inkB, *gnd;
    int x, y;
    (void)sl;

    if (!p114_tab) {
        int q;
        for (q = 0; q < 1024; q++)
            p114_sin[q] = sinf((float)q * 6.28318530718f / 1024.0f);
        p114_tab = 1;
    }
    if (w != p114_w || h != p114_h) p114_grid(w, h);
    p114_ramp_build(pal);

    {
        float rot = 0.00040f * t + sp;
        ca = cosf(rot); sa = sinf(rot);
    }
    cell  = (float)h * (0.150f + 0.012f * sinf(0.00058f * t));
    inv   = 1.0f / cell;
    rad   = 0.452f + 0.026f * sinf(0.00047f * t + sp);
    wcore = 0.036f;
    wedge = 0.062f;
    kcore = cell * 0.5f;                    /* edge softness ~2 px */
    kedge = cell * 0.5f;
    glow  = 0.00207f * t; glow -= floorf(glow);
    {
        int hi = (int)(t * 0.040f + sp * 40.0f) & 255;
        inkA = p114_ramp[hi];
        inkB = p114_ramp[(hi + 88) & 255];
        gnd  = p114_ramp[(hi + 168) & 255];
    }

    for (y = 0; y < h; y++) {
        float py = p114_dy[y];
        float qx = (p114_dx[0] * ca - py * sa) * inv;
        float qy = (p114_dx[0] * sa + py * ca) * inv;
        float dqx = ca * inv, dqy = sa * inv;
        uint32_t *dst = fb + (size_t)y * (size_t)w;
        /* the cell fold is carried incrementally along the row: no floor, no
         * float-to-int conversion in the inner loop at all */
        int cxA = (int)floorf(qx + 0.5f), cyA = (int)floorf(qy + 0.5f);
        float ax = qx - (float)cxA, ay = qy - (float)cyA;
        for (x = 0; x < w; x++) {
            float bx, by, dA, dB, eA, eB;
            float cA, cB, gA, gB, ri, gi, bi;
            int top;
            float rr, gg, bb;

            /* the offset lattice is the same fold shifted half a cell */
            bx = ax >= 0.0f ? ax - 0.5f : ax + 0.5f;
            by = ay >= 0.0f ? ay - 0.5f : ay + 0.5f;

            {
                float u = ax < 0.0f ? -ax : ax, v = ay < 0.0f ? -ay : ay;
                float m = u > v ? u : v, s = (u + v) * P114_R2;
                dA = m > s ? m : s;
            }
            {
                float u = bx < 0.0f ? -bx : bx, v = by < 0.0f ? -by : by;
                float m = u > v ? u : v, s = (u + v) * P114_R2;
                dB = m > s ? m : s;
            }
            eA = dA - rad; if (eA < 0.0f) eA = -eA;
            eB = dB - rad; if (eB < 0.0f) eB = -eB;
            cA = (wcore - eA) * kcore; if (cA > 1.0f) cA = 1.0f; if (cA < 0.0f) cA = 0.0f;
            cB = (wcore - eB) * kcore; if (cB > 1.0f) cB = 1.0f; if (cB < 0.0f) cB = 0.0f;
            gA = (wedge - eA) * kedge; if (gA > 1.0f) gA = 1.0f; if (gA < 0.0f) gA = 0.0f;
            gB = (wedge - eB) * kedge; if (gB > 1.0f) gB = 1.0f; if (gB < 0.0f) gB = 0.0f;

            /* light running along each strap */
            {
                float pa = (ax + ay) * 0.859f + (float)((cxA + cyA) & 7) * 0.302f
                           - glow + 8.0f;
                float pb = (bx - by) * 0.859f + (float)((cxA - cyA) & 7) * 0.302f
                           + glow + 8.0f;
                int ia = (int)(pa * 1024.0f) & 1023;
                int ib = (int)(pb * 1024.0f) & 1023;
                cA *= 0.62f + 0.38f * p114_sin[ia];
                cB *= 0.62f + 0.38f * p114_sin[ib];
            }

            top = ((cxA + cyA) & 1);        /* which family passes over here */
            rr = gnd[0] * 0.07f; gg = gnd[1] * 0.07f; bb = gnd[2] * 0.07f;
            if (top) {
                if (gB > 0.0f) { rr = inkB[0] * cB * gB; gg = inkB[1] * cB * gB;
                                 bb = inkB[2] * cB * gB; }
                else if (gA > 0.0f) { rr = inkA[0] * cA * gA; gg = inkA[1] * cA * gA;
                                      bb = inkA[2] * cA * gA; }
            } else {
                if (gA > 0.0f) { rr = inkA[0] * cA * gA; gg = inkA[1] * cA * gA;
                                 bb = inkA[2] * cA * gA; }
                else if (gB > 0.0f) { rr = inkB[0] * cB * gB; gg = inkB[1] * cB * gB;
                                      bb = inkB[2] * cB * gB; }
            }
            ri = rr * 255.0f; gi = gg * 255.0f; bi = bb * 255.0f;
            {
                int R = ri > 255.0f ? 255 : (int)ri;
                int G = gi > 255.0f ? 255 : (int)gi;
                int B = bi > 255.0f ? 255 : (int)bi;
                dst[x] = 0xFF000000u | ((uint32_t)R << 16) |
                         ((uint32_t)G << 8) | (uint32_t)B;
            }
            ax += dqx;
            if (ax > 0.5f) { ax -= 1.0f; cxA++; }
            else if (ax < -0.5f) { ax += 1.0f; cxA--; }
            ay += dqy;
            if (ay > 0.5f) { ay -= 1.0f; cyA++; }
            else if (ay < -0.5f) { ay += 1.0f; cyA--; }
        }
    }
}
