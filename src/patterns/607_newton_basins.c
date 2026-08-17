/* 607 Newton Basins — the map of where an answer comes from.
 *
 * Newton's method solves z^n = 1 by repeatedly stepping z -> z - f(z)/f'(z).
 * From most starting points it lands on one of the n roots. Colour every point
 * of the plane by WHICH root it reached, and the boundaries between those
 * territories turn out to be infinitely intricate — every point on a border
 * touches all n basins at once. It is the oldest picture in this whole family
 * (Cayley posed it in 1879) and it looks like nothing else here: interlocking
 * bulbs, not filaments or rigging.
 *
 * Two things make it a good fit for this engine:
 *
 *   - It is CHEAP. Newton converges in a couple of dozen steps, not hundreds,
 *     so unlike 604/606 this one does not have to be frozen after the first
 *     render. It is re-swept continuously — a band of scanlines every frame,
 *     top to bottom, forever.
 *
 *   - It MORPHS. Relaxing the step to z -> z - a·f/f' with a complex `a` bends
 *     the basins: at a = 1 they are the clean symmetric flower, and as `a`
 *     walks off into the complex plane the territories spiral into each other
 *     and grow tendrils. `a` drifts a little each sweep, so the shape is always
 *     becoming something else rather than sitting still.
 *
 * Colour: root index picks a region of the ramp, convergence speed shades
 * within it. Nothing is black — the slowest-converging points, which is what
 * the border is made of, get the brightest part of their band.
 */
#include "../engine/jellydazzle.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

#define N607_ROWS 9               /* scanlines re-swept per frame */
#define N607_MAXIT 32

static uint8_t *pl607;
static int      pw607, ph607;
static uint32_t seed607 = 0xFFFFFFFFu;
static int      row607;
static double   ph607a;           /* where `a` is on its walk */
static double   p607[8];
static int      deg607;

static uint32_t r607(uint32_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
    return *s;
}
static float f607(uint32_t *s) { return (float)(r607(s) >> 8) * (1.0f / 16777216.0f); }

static void plan607(int w, int h, uint32_t seed)
{
    size_t need = (size_t)w * h;
    if (pw607 != w || ph607 != h) {
        free(pl607); pl607 = (uint8_t *)malloc(need);
        pw607 = w; ph607 = h;
    }
    if (!pl607) return;
    memset(pl607, 0, need);
    row607 = 0;

    uint32_t s = seed ? seed : 0x1879CA1Eu;
    deg607   = 3 + (int)(f607(&s) * 6.0f);          /* z^3..z^8 = 3..8 basins */
    if (deg607 > 8) deg607 = 8;
    ph607a   = (double)f607(&s) * 6.28318530718;
    p607[0]  = 1.10 + 1.40 * (double)f607(&s);      /* view half-height       */
    p607[1]  = (double)f607(&s) * 6.28318530718;    /* view rotation          */
    p607[2]  = 8.0 + (double)f607(&s) * 220.0;      /* palette base           */
    p607[3]  = 0.0026 + 0.0060 * (double)f607(&s);  /* morph speed            */
    p607[4]  = 0.30 + 0.55 * (double)f607(&s);      /* how far `a` wanders    */
    p607[5]  = 0.55 + 0.85 * (double)f607(&s);      /* zoom / drift amount    */
    p607[6]  = (double)f607(&s);                    /* shading depth          */
}

static void slice607(int w, int h, int rows)
{
    if (!pl607) return;
    int    n     = deg607;
    double half  = p607[0] * (1.0 + p607[5] * 0.26 * sin(ph607a * 0.71));
    double vr    = p607[1];
    int    base  = (int)p607[2];
    double wander= p607[4];
    double shade = 6.0 + 14.0 * p607[6];

    /* the relaxation constant, walking a small loop around 1 */
    double ar = 1.0 + wander * cos(ph607a);
    double ai =       wander * sin(ph607a * 1.31);

    double offr = p607[5] * 0.20 * sin(ph607a * 0.47);
    double offi = p607[5] * 0.20 * cos(ph607a * 0.33 + 0.9);

    double cs = cos(vr), sn = sin(vr);
    double aspect = (double)w / (double)h;
    int    span = 256 / n;          /* ramp region per basin */

    int yend = row607 + rows;
    if (yend > h) yend = h;
    for (int y = row607; y < yend; y++) {
        double v = ((double)y / (double)h - 0.5) * 2.0 * half;
        uint8_t *dst = pl607 + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            double u = ((double)x / (double)w - 0.5) * 2.0 * half * aspect;
            double zr = u * cs - v * sn + offr;
            double zi = u * sn + v * cs + offi;

            int it = 0; double last = 1.0;
            for (; it < N607_MAXIT; it++) {
                /* p = z^n and q = z^(n-1), by repeated multiplication —
                 * cheaper and better conditioned than pow() on complexes */
                double qr = 1.0, qi = 0.0;
                for (int k = 0; k < n - 1; k++) {
                    double t = qr * zr - qi * zi;
                    qi = qr * zi + qi * zr;
                    qr = t;
                }
                double pr = qr * zr - qi * zi;      /* z^n     */
                double pi = qr * zi + qi * zr;
                pr -= 1.0;                          /* z^n - 1 */

                /* denominator n·z^(n-1) */
                double dr = qr * (double)n, di = qi * (double)n;
                double dd = dr * dr + di * di;
                if (dd < 1e-18) break;              /* at the origin: no step  */

                /* step = a · (z^n - 1) / (n z^(n-1)) */
                double nr = (pr * dr + pi * di) / dd;
                double ni = (pi * dr - pr * di) / dd;
                double sr = ar * nr - ai * ni;
                double si = ar * ni + ai * nr;

                zr -= sr; zi -= si;
                last = sr * sr + si * si;
                if (last < 1e-14) { it++; break; }
            }

            /* which root did it land on? roots of z^n=1 are at angle 2πk/n */
            double ang = atan2(zi, zr);
            if (ang < 0.0) ang += 6.28318530718;
            int k = (int)(ang * (double)n / 6.28318530718 + 0.5) % n;

            /* Root picks the band, convergence speed shades inside it. The raw
             * step count is a small integer — Newton converges in a handful of
             * steps — so on its own it leaves each basin a flat poster block.
             * Folding in how far the LAST step travelled makes it continuous,
             * and the basins get an interior gradient. Slow points — the
             * fractal border — sit at the far end of the band, so the boundary
             * reads as a bright seam instead of a black one. */
            double smooth = (double)it - log(log(last + 1e-300) / -32.236) * 1.4;
            if (smooth < 0.0) smooth = 0.0;
            /* Map the convergence range ACROSS the band rather than scaling by
             * a constant — a constant saturates within two or three steps and
             * leaves the basin flat again, which is what the first attempt did.
             * The sqrt gives the fast-converging bulk of each basin more of the
             * band, so the gradient is visible where most of the pixels are. */
            double frac = smooth * (1.0 / (double)N607_MAXIT);
            if (frac > 1.0) frac = 1.0;
            int within = (int)(sqrt(frac) * shade * (double)(span - 1) * (1.0 / 20.0));
            if (within > span - 1) within = span - 1;
            dst[x] = (uint8_t)((base + k * span + within) & 255);
        }
    }
    row607 = yend;
    if (row607 >= h) {
        row607 = 0;
        ph607a += p607[3] * 20.0;
        if (ph607a > 6.28318530718) ph607a -= 6.28318530718;
    }
}

void pattern_607(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (seed != seed607 || pw607 != w || ph607 != h) {
        plan607(w, h, seed);
        seed607 = seed;
    }
    if (!pl607) return;
    slice607(w, h, N607_ROWS);

    uint32_t rot = (uint32_t)frame * 15u;
    uint32_t lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = pal[((uint32_t)i * 128u + rot) & JD_PAL_MASK];
    const uint8_t *src = pl607;
    size_t n = (size_t)w * h;
    for (size_t i = 0; i < n; i++) fb[i] = lut[src[i]];
}
