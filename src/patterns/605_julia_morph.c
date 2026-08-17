/* 605 Julia Morph — a fractal that changes shape while you watch.
 *
 * A Julia set is z -> z^2 + c for a FIXED c, and the shape it makes is
 * extraordinarily sensitive to that c: move it a hair and a connected blob
 * becomes a dust of islands, a spiral becomes a dendrite. That sensitivity is
 * usually a nuisance. Here it is the whole point.
 *
 * The trick is affording it. Recomputing a full frame costs ~300 ms, which is
 * an eighteen-frame stall, so a naive morph would be a slideshow. Instead the
 * plane is recomputed CONTINUOUSLY, a few scanlines per frame, sweeping top to
 * bottom forever. Each complete sweep uses a slightly advanced c. So the new
 * shape wipes down over the old one, endlessly, and the fractal is always in
 * the middle of becoming something else.
 *
 * Between sweeps the palette moves underneath — the Dazzle method — so colour
 * travels even where the geometry has not been rewritten yet.
 *
 * c walks a closed loop rather than drifting randomly. A random walk wanders
 * into the solid interior (a black screen) or the far exterior (empty dust).
 * The loop is chosen to stay in the interesting band near the boundary of the
 * Mandelbrot set, which is exactly where Julia sets are beautiful.
 */
#include "../engine/jellydazzle.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

#define J605_BG 0
#define J605_ROWS 14              /* scanlines recomputed per frame */

static uint8_t *pl605;
static int      pw605, ph605;
static uint32_t seed605 = 0xFFFFFFFFu;
static int      row605;
static double   ang605;           /* where we are around the c-loop */
static double   p605[8];

static uint32_t r605(uint32_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
    return *s;
}
static float f605(uint32_t *s) { return (float)(r605(s) >> 8) * (1.0f / 16777216.0f); }

/* Known-good c values, and why a formula did not work.
 *
 * First attempt traced the cardioid boundary analytically and pushed outward
 * by a small radius. Half the time the wobble pushed INWARD, landing c inside
 * the main cardioid — where the Julia set is a fat filled blob whose interior
 * has no escape data, so the screen went black. The other half landed far
 * outside, where the set is Cantor dust and the screen went empty.
 *
 * The band that is actually beautiful is very thin. Rather than hunt it with
 * maths, these are hand-picked c values, each a well-known Julia: dendrite,
 * rabbit, san marco, siegel disk, spiral. Morphing means interpolating from
 * one to the next, so the shape genuinely transforms — a rabbit uncurling
 * into a dendrite — instead of wandering into nothing. */
static const struct { double r, i; } J605_C[] = {
    { -0.4,      0.6      },   /* the classic swirl                */
    { -0.70176, -0.3842   },   /* seahorse-ish, delicate arms      */
    { -0.835,   -0.2321   },   /* dense filigree                   */
    { -0.7269,   0.1889   },   /* dendrite lace                    */
    {  0.285,    0.01     },   /* near-parabolic, fine spirals     */
    { -0.123,    0.745    },   /* Douady rabbit — three-fold       */
    { -0.75,     0.11     },   /* san marco, budding               */
    { -0.391,   -0.587    },   /* siegel disk, smooth whorls       */
    {  0.0,      0.8      },   /* dendrite, sparse and branching   */
    { -1.037,    0.17     },   /* long thin tendrils               */
};
#define J605_NC ((int)(sizeof J605_C / sizeof J605_C[0]))

/* smooth interpolation around the loop: t in [0, NC) */
static void c_at(double t, double push, double *cr, double *ci)
{
    (void)push;
    while (t < 0.0) t += (double)J605_NC;
    while (t >= (double)J605_NC) t -= (double)J605_NC;
    int a = (int)t; int b = (a + 1) % J605_NC;
    double f = t - (double)a;
    f = f * f * (3.0 - 2.0 * f);                 /* ease, so it never snaps */
    *cr = J605_C[a].r + (J605_C[b].r - J605_C[a].r) * f;
    *ci = J605_C[a].i + (J605_C[b].i - J605_C[a].i) * f;
}

static void plan605(int w, int h, uint32_t seed)
{
    size_t need = (size_t)w * h;
    if (pw605 != w || ph605 != h) {
        free(pl605); pl605 = (uint8_t *)malloc(need);
        pw605 = w; ph605 = h;
    }
    if (!pl605) return;
    memset(pl605, J605_BG, need);
    row605 = 0;

    uint32_t s = seed ? seed : 0x9E3779B9u;
    ang605   = (double)f605(&s) * (double)J605_NC;
    p605[0]  = 1.30 + 0.95 * (double)f605(&s);          /* view half-height   */
    p605[1]  = 140.0 + (double)f605(&s) * 220.0;        /* iterations         */
    p605[2]  = 3.0 + (double)f605(&s) * 8.0;            /* index step         */
    p605[3]  = 8.0 + (double)f605(&s) * 220.0;          /* palette base       */
    p605[4]  = (double)f605(&s) * 6.28318530718;        /* view rotation      */
    p605[5]  = 0.0012 + 0.0026 * (double)f605(&s);      /* morph speed        */
    p605[6]  = 0.008 + 0.030 * (double)f605(&s);        /* push off the edge  */
    p605[7]  = 0.55 + 0.9 * (double)f605(&s);           /* zoom drift amount  */
}

static void slice605(int w, int h, int rows)
{
    if (!pl605) return;
    /* Framing is not fixed. The plane is rebuilt every sweep anyway, so a slow
     * breathing zoom and drift cost nothing and mean the set is never twice at
     * the same size in the same place. */
    double half   = p605[0] * (1.0 + p605[7] * 0.30 * sin(ang605 * 0.83 + p605[4]));
    double offr   = p605[7] * 0.22 * sin(ang605 * 0.51);
    double offi   = p605[7] * 0.22 * cos(ang605 * 0.37 + 1.1);
    int    iters  = (int)p605[1];
    int    band   = (int)p605[2];
    int    base   = (int)p605[3];
    double vr     = p605[4];
    double push   = p605[6];

    double cr, ci;
    c_at(ang605, push, &cr, &ci);
    double cs = cos(vr), sn = sin(vr);
    double aspect = (double)w / (double)h;

    int yend = row605 + rows;
    if (yend > h) yend = h;
    for (int y = row605; y < yend; y++) {
        double v = ((double)y / (double)h - 0.5) * 2.0 * half;
        uint8_t *dst = pl605 + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            double u = ((double)x / (double)w - 0.5) * 2.0 * half * aspect;
            double zr = u * cs - v * sn + offr;
            double zi = u * sn + v * cs + offi;
            double zr2 = zr * zr, zi2 = zi * zi;
            double trap = 1e30;                  /* closest approach to origin */
            double xtrap = 1e30;                 /* closest approach to the axes */
            int i = 0;
            while (i < iters && zr2 + zi2 <= 65536.0) {
                zi = 2.0 * zr * zi + ci;
                zr = zr2 - zi2 + cr;
                zr2 = zr * zr; zi2 = zi * zi;
                double d = zr2 + zi2;
                if (d < trap) trap = d;
                double ax = fabs(zr) < fabs(zi) ? fabs(zr) : fabs(zi);
                if (ax < xtrap) xtrap = ax;
                i++;
            }
            int idx;
            if (i >= iters) {
                /* INSIDE the set. There is no escape count here, so the usual
                 * answer is to paint it black — and a black hole in the middle
                 * of the screen is exactly what we do not want. Instead use an
                 * ORBIT TRAP: how close the orbit ever came to the origin. That
                 * varies smoothly across the interior and carries real
                 * structure (the internal bulbs and spirals), so the inside of
                 * the set gets its own full sweep of hue rather than a hole. */
                double t = log(sqrt(trap) + 1e-12);          /* ~ -14 .. 0 */
                idx = base + 128 + (int)(t * -14.0 * (double)band);
            } else {
                /* OUTSIDE. The smooth escape count alone gives clean but very
                 * plain contour rings — a vignette with a jewel in the middle.
                 * Folding in the same orbit trap adds the filaments that reach
                 * out from the set, so the exterior carries structure too and
                 * the whole frame is doing something. */
                double mag = sqrt(zr2 + zi2);
                double nu  = (double)i + 1.0 - log(log(mag) / log(2.0)) / log(2.0);
                double t   = log(xtrap + 1e-12);
                idx = base + (int)(nu * (double)band) + (int)(t * -13.0);
            }
            dst[x] = (uint8_t)(idx & 255);
        }
    }
    row605 = yend;
    if (row605 >= h) {              /* sweep finished: advance c, start again */
        row605 = 0;
        ang605 += p605[5] * 22.0;     /* one c-to-c morph ~20 s at 60fps */
        if (ang605 >= (double)J605_NC) ang605 -= (double)J605_NC;
    }
}

void pattern_605(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (seed != seed605 || pw605 != w || ph605 != h) {
        plan605(w, h, seed);
        seed605 = seed;
    }
    if (!pl605) return;
    slice605(w, h, J605_ROWS);      /* always rebuilding — it never settles */

    /* No index is reserved for black. Every one of the 256 slots maps into the
     * live palette, so the whole plane — inside the set and out — is colour. */
    uint32_t rot = (uint32_t)frame * 17u;
    uint32_t lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = pal[((uint32_t)i * 128u + rot) & JD_PAL_MASK];
    const uint8_t *src = pl605;
    size_t n = (size_t)w * h;
    for (size_t i = 0; i < n; i++) fb[i] = lut[src[i]];
}
