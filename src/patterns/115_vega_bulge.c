/* 115 Vega Bulge — Vasarely op-art. A flat lattice of cells is pushed through a
 * radial magnification field s(r) = 1 + A/(1 + (r/sigma)^2): every pixel is
 * mapped back through that field before the lattice is looked up, so the cells
 * swell toward the centre of the bulge and crowd at its rim, and the flat plane
 * reads as a sphere pressing out of the picture. Cells are drawn with a soft
 * rounded-square profile whose radius follows the local magnification, which is
 * what sells the volume. One bulge and one pinch drift on slow Lissajous paths
 * and breathe; the lattice itself rotates about a degree every ten seconds.
 * Colour is a checkerboard of two palette hues over a dark ground —
 * deliberately stark. Full-bleed: a ground layer.
 */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

static float *p115_dx, *p115_dy;
static int p115_w = -1, p115_h = -1;
static float p115_ramp[256][3];
static float p115_inkv[3], p115_ink2v[3], p115_gndv[3];

static void p115_ramp_build(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i * 128) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 8.0f) mx = 8.0f;
        p115_ramp[i][0] = r / mx; p115_ramp[i][1] = g / mx; p115_ramp[i][2] = b / mx;
    }
}

static void p115_grid(int w, int h)
{
    int x, y;
    float cx = (float)w * 0.5f, cy = (float)h * 0.5f;
    free(p115_dx); free(p115_dy);
    p115_dx = (float *)malloc(sizeof(float) * (size_t)w);
    p115_dy = (float *)malloc(sizeof(float) * (size_t)h);
    for (x = 0; x < w; x++) p115_dx[x] = (float)x + 0.5f - cx;
    for (y = 0; y < h; y++) p115_dy[y] = (float)y + 0.5f - cy;
    p115_w = w; p115_h = h;
}

void pattern_115(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sp = (float)(seed & 1023) * 0.006136f;
    float ca, sa, cell, inv, b1x, b1y, b1a, b1s, b2x, b2y, b2a, b2s;
    const float *ink, *ink2, *gnd;
    int x, y;
    (void)sl;

    if (w != p115_w || h != p115_h) p115_grid(w, h);
    p115_ramp_build(pal);

    {
        float rot = 0.00042f * t + sp;
        ca = cosf(rot); sa = sinf(rot);
    }
    cell = (float)h * (0.058f + 0.006f * sinf(0.00071f * t));
    inv  = 1.0f / cell;
    b1x = (float)w * 0.30f * sinf(0.00053f * t + sp);
    b1y = (float)h * 0.28f * sinf(0.00041f * t + 1.9f);
    b1a = 1.35f + 0.55f * sinf(0.00061f * t);
    b1s = (float)h * (0.30f + 0.06f * sinf(0.00037f * t + 2.2f));
    b2x = (float)w * 0.32f * sinf(0.00033f * t + 2.7f);
    b2y = (float)h * 0.30f * sinf(0.00047f * t + 0.6f);
    b2a = -0.62f - 0.25f * sinf(0.00044f * t + 1.1f);   /* a pinch, not a bulge */
    b2s = (float)h * (0.24f + 0.05f * sinf(0.00029f * t));
    b1s = 1.0f / (b1s * b1s);
    b2s = 1.0f / (b2s * b2s);
    {
        /* Ping-pong, not wrap (see note above) — and, TEMPORAL REVIEW
         * 2.4.0 (docs/review/04_pattern_temporal.md, F-11x): the index
         * used to STEP one ramp entry every ~26 frames, snapping all
         * three colours at once.  Walk the triangle in float and lerp
         * the two neighbouring ramp entries so the inks glide. */
        float h0 = t * 0.038f + sp * 40.0f;
        int k;
        { float q = h0 + 0.0f;
          q = q - 512.0f * floorf(q * (1.0f / 512.0f));
          if (q > 255.5f) q = 511.0f - q;   /* triangle, may dip <0 */
          if (q < 0.0f) q = 0.0f;
          k = (int)q;
          { float f = q - (float)k; int k1 = k + 1 < 256 ? k + 1 : 255;
            for (int c3 = 0; c3 < 3; c3++)
                p115_inkv[c3] = p115_ramp[k][c3] + (p115_ramp[k1][c3] - p115_ramp[k][c3]) * f; } }
        { float q = h0 + 70.0f;
          q = q - 512.0f * floorf(q * (1.0f / 512.0f));
          if (q > 255.5f) q = 511.0f - q;   /* triangle, may dip <0 */
          if (q < 0.0f) q = 0.0f;
          k = (int)q;
          { float f = q - (float)k; int k1 = k + 1 < 256 ? k + 1 : 255;
            for (int c3 = 0; c3 < 3; c3++)
                p115_ink2v[c3] = p115_ramp[k][c3] + (p115_ramp[k1][c3] - p115_ramp[k][c3]) * f; } }
        { float q = h0 + 160.0f;
          q = q - 512.0f * floorf(q * (1.0f / 512.0f));
          if (q > 255.5f) q = 511.0f - q;   /* triangle, may dip <0 */
          if (q < 0.0f) q = 0.0f;
          k = (int)q;
          { float f = q - (float)k; int k1 = k + 1 < 256 ? k + 1 : 255;
            for (int c3 = 0; c3 < 3; c3++)
                p115_gndv[c3] = p115_ramp[k][c3] + (p115_ramp[k1][c3] - p115_ramp[k][c3]) * f; } }
    }
    ink = p115_inkv; ink2 = p115_ink2v; gnd = p115_gndv;

    for (y = 0; y < h; y++) {
        float py = p115_dy[y];
        uint32_t *dst = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            float px = p115_dx[x];
            float e1x = px - b1x, e1y = py - b1y;
            float e2x = px - b2x, e2y = py - b2y;
            float s = 1.0f + b1a / (1.0f + (e1x * e1x + e1y * e1y) * b1s)
                           + b2a / (1.0f + (e2x * e2x + e2y * e2y) * b2s);
            float qx, qy, ux, uy, fx, fy, d, rad, v, ri, gi, bi;
            int ix, iy;
            const float *col;
            if (s < 0.22f) s = 0.22f;
            qx = (px * ca - py * sa) / s;
            qy = (px * sa + py * ca) / s;
            ux = qx * inv; uy = qy * inv;
            ix = (int)floorf(ux); iy = (int)floorf(uy);
            fx = ux - (float)ix - 0.5f; fy = uy - (float)iy - 0.5f;
            /* rounded-square cell: superellipse distance, radius from the
             * local magnification so swollen cells read as raised */
            d = sqrtf(sqrtf(fx * fx * fx * fx + fy * fy * fy * fy));
            rad = 0.30f + 0.14f * (s - 1.0f);
            if (rad < 0.10f) rad = 0.10f;
            if (rad > 0.46f) rad = 0.46f;
            v = (rad - d) * (7.0f + 3.0f * s);
            if (v > 1.0f) v = 1.0f; else if (v < 0.0f) v = 0.0f;
            col = ((ix + iy) & 1) ? ink : ink2;
            {
                float sh = 0.55f + 0.27f * s;               /* bulge shading */
                if (sh > 1.25f) sh = 1.25f;
                ri = (col[0] * v * sh + gnd[0] * (1.0f - v) * 0.11f) * 255.0f;
                gi = (col[1] * v * sh + gnd[1] * (1.0f - v) * 0.11f) * 255.0f;
                bi = (col[2] * v * sh + gnd[2] * (1.0f - v) * 0.11f) * 255.0f;
            }
            {
                int R = ri > 255.0f ? 255 : (int)ri;
                int G = gi > 255.0f ? 255 : (int)gi;
                int B = bi > 255.0f ? 255 : (int)bi;
                dst[x] = 0xFF000000u | ((uint32_t)R << 16) |
                         ((uint32_t)G << 8) | (uint32_t)B;
            }
        }
    }
}
