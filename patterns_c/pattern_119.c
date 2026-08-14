/* 119 Rose Window — Gothic plate tracery, lit from behind.
 * Three rings of foils (12 large, 24 small, 6 in the medallion) are tested by
 * folding the pixel's angle into one sector per ring and using the law of
 * cosines: d^2 = r^2 + dp^2 - 2 r dp cos(a). Comparing d^2 against (rp+-w)^2
 * keeps the whole test free of square roots, so the entire window — foils,
 * cusps, radial mullions, the concentric stone courses and the outer frame —
 * costs about thirty operations per pixel from two polar tables. Stone is near
 * black; every glass cell takes its hue from its ring and sector, and a slow
 * radial wave of light travels outward through the glass as if the sun were
 * moving behind it. The whole window turns once every twenty minutes.
 * Full-bleed and saturated: a ground layer.
 */
#include "../jellydazzle.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#define P119_TAU 6.28318530717959f

static float *p119_r, *p119_th;
static int p119_w = -1, p119_h = -1;
static float p119_cos[1024];
static int p119_tab;
static float p119_ramp[256][3];

static void p119_ramp_build(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i * 128) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 8.0f) mx = 8.0f;
        p119_ramp[i][0] = r / mx; p119_ramp[i][1] = g / mx; p119_ramp[i][2] = b / mx;
    }
}

static void p119_polar(int w, int h)
{
    int x, y;
    float cx = (float)w * 0.5f, cy = (float)h * 0.5f;
    free(p119_r); free(p119_th);
    p119_r  = (float *)malloc(sizeof(float) * (size_t)w * (size_t)h);
    p119_th = (float *)malloc(sizeof(float) * (size_t)w * (size_t)h);
    for (y = 0; y < h; y++) {
        float dy = (float)y + 0.5f - cy;
        for (x = 0; x < w; x++) {
            float dx = (float)x + 0.5f - cx;
            size_t o = (size_t)y * (size_t)w + (size_t)x;
            p119_r[o] = sqrtf(dx * dx + dy * dy);
            p119_th[o] = atan2f(dy, dx);
        }
    }
    p119_w = w; p119_h = h;
}

void pattern_119(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    const int nA = 12, nB = 24, nC = 6;
    float t = (float)frame;
    float sp = (float)(seed & 1023) * 0.006136f;
    float R, rot, stw, wave;
    float dpA, rpA, dpB, rpB, dpC, rpC;
    float sA, sB, sC, iA, iB, iC;
    float cA0, cA1, cB0, cB1, cC0, cC1;
    float rings[4], ringw;
    int x, y, i, hbase;
    (void)sl;

    if (!p119_tab) {
        for (i = 0; i < 1024; i++)
            p119_cos[i] = cosf((float)i * P119_TAU / 1024.0f);
        p119_tab = 1;
    }
    if (w != p119_w || h != p119_h) p119_polar(w, h);
    p119_ramp_build(pal);

    R   = (float)h * 0.485f;
    rot = 0.00052f * t + sp;
    stw = R * 0.016f;                       /* stone bar half-width */
    wave = 0.010f * t;
    dpA = R * (0.605f + 0.012f * sinf(0.00061f * t));
    rpA = R * (0.255f + 0.010f * sinf(0.00061f * t));
    dpB = R * 0.870f;
    rpB = R * (0.118f + 0.006f * sinf(0.00047f * t + 1.4f));
    dpC = R * 0.120f;
    rpC = R * 0.072f;
    sA = P119_TAU / (float)nA; iA = 1.0f / sA;
    sB = P119_TAU / (float)nB; iB = 1.0f / sB;
    sC = P119_TAU / (float)nC; iC = 1.0f / sC;
    cA0 = (rpA - stw) * (rpA - stw); cA1 = (rpA + stw) * (rpA + stw);
    cB0 = (rpB - stw) * (rpB - stw); cB1 = (rpB + stw) * (rpB + stw);
    cC0 = (rpC - stw) * (rpC - stw); cC1 = (rpC + stw) * (rpC + stw);
    rings[0] = R * 0.215f; rings[1] = R * 0.735f;
    rings[2] = R * 0.965f; rings[3] = R * 1.000f;
    ringw = stw;
    hbase = (int)(t * 0.037f + sp * 30.0f);

    for (y = 0; y < h; y++) {
        const float *rr = p119_r + (size_t)y * (size_t)w;
        const float *tt = p119_th + (size_t)y * (size_t)w;
        uint32_t *dst = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            float r = rr[x], th = tt[x] + rot;
            float r2 = r * r;
            float aA, aB, aC, dA, dB, dC;
            int kA, kB, kC, stone = 0, hi;
            float v, gl;
            const float *col;
            float ri, gi, bi;

            if (r > R * 1.02f) { dst[x] = 0xFF000000u; continue; }

            /* fold the angle into one sector per ring */
            {
                float q = th * iA; kA = (int)floorf(q + 0.5f); aA = th - (float)kA * sA;
                q = th * iB; kB = (int)floorf(q + 0.5f); aB = th - (float)kB * sB;
                q = th * iC; kC = (int)floorf(q + 0.5f); aC = th - (float)kC * sC;
            }
            {
                int j;
                j = (int)(aA * (1024.0f / P119_TAU) + 1024.5f) & 1023;
                dA = r2 + dpA * dpA - 2.0f * r * dpA * p119_cos[j];
                j = (int)(aB * (1024.0f / P119_TAU) + 1024.5f) & 1023;
                dB = r2 + dpB * dpB - 2.0f * r * dpB * p119_cos[j];
                j = (int)(aC * (1024.0f / P119_TAU) + 1024.5f) & 1023;
                dC = r2 + dpC * dpC - 2.0f * r * dpC * p119_cos[j];
            }

            /* stone: foil outlines, radial mullions, concentric courses */
            if (dA > cA0 && dA < cA1 && r > rings[0]) stone = 1;
            if (dB > cB0 && dB < cB1 && r > rings[1] * 0.92f) stone = 1;
            if (dC > cC0 && dC < cC1 && r < rings[0]) stone = 1;
            {
                float ang = aA < 0.0f ? -aA : aA;
                if (ang * r < stw && r > rings[0]) stone = 1;
            }
            for (i = 0; i < 4; i++) {
                float e = r - rings[i];
                if (e < 0.0f) e = -e;
                if (e < ringw) stone = 1;
            }
            if (r > R) stone = 1;

            /* glass cell identity */
            if (r < rings[0]) {
                hi = (dC < cC0) ? (hbase + 30 + kC * 9) : (hbase + 96);
            } else if (dA < cA0) {
                hi = hbase + 8 + kA * 17;
            } else if (dB < cB0) {
                hi = hbase + 150 + kB * 6;
            } else {
                int band = (int)((r - rings[0]) * (5.0f / (R - rings[0])));
                hi = hbase + 190 + band * 11 + (kA & 1) * 24;
            }
            col = p119_ramp[hi & 255];
            gl = 0.55f + 0.45f * p119_cos[(int)((r * 0.028f - wave) * 162.97f
                                                + 4096.5f) & 1023];
            v = stone ? 0.055f : (0.60f + 0.55f * gl);
            ri = col[0] * v * 255.0f;
            gi = col[1] * v * 255.0f;
            bi = col[2] * v * 255.0f;
            {
                int Rr = ri > 255.0f ? 255 : (int)ri;
                int Gg = gi > 255.0f ? 255 : (int)gi;
                int Bb = bi > 255.0f ? 255 : (int)bi;
                dst[x] = 0xFF000000u | ((uint32_t)Rr << 16) |
                         ((uint32_t)Gg << 8) | (uint32_t)Bb;
            }
        }
    }
}
