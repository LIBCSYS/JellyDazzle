/* 606 Burning Ship — the Mandelbrot's angrier cousin.
 *
 * One character changed from z -> z^2 + c: take the ABSOLUTE VALUE of both
 * parts of z before squaring. That single fold destroys the symmetry the
 * Mandelbrot has and produces something that looks like a ship on fire, all
 * masts and rigging and sharp diagonal spars — a completely different visual
 * vocabulary from anything else in the library, which is the whole reason to
 * carry it.
 *
 * Same economics as 604: expensive to compute, free to animate. Rendered once
 * into a plane of palette indices, then the palette moves underneath for the
 * rest of the turn. Progressive build, so it resolves downward into view over
 * about a second instead of stalling the frame.
 *
 * Colouring, per J: nothing is painted black. The interior is coloured by an
 * orbit trap — how close the orbit ever came to the origin — so the inside of
 * the hull is a gradient rather than a hole.
 */
#include "../engine/jellydazzle.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

/* Places on the ship worth standing. The full view is mostly empty sea; the
 * structure lives along the hull and out on the antenna to the left. */
static const struct { double cx, cy, span; } B606_SPOT[] = {
    { -1.7548,   -0.0100,  0.09000 },   /* the whole ship, masts and all   */
    { -1.7753,   -0.0035,  0.00450 },   /* the antenna tip                 */
    { -1.7690,   -0.0400,  0.02000 },   /* rigging off the bow             */
    { -1.5800,   -0.0350,  0.06000 },   /* the smaller ship astern         */
    { -1.7430,   -0.0280,  0.00800 },   /* hull filigree                   */
    { -0.4500,    0.6000,  0.35000 },   /* the upper archipelago           */
    { -1.9400,    0.0000,  0.05000 },   /* far bowsprit, spiky             */
};
#define B606_NSPOT ((int)(sizeof B606_SPOT / sizeof B606_SPOT[0]))

static uint8_t *pl606;
static int      pw606, ph606;
static uint32_t seed606 = 0xFFFFFFFFu;
static int      row606;
static double   par606[8];

static uint32_t r606(uint32_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
    return *s;
}
static float f606(uint32_t *s) { return (float)(r606(s) >> 8) * (1.0f / 16777216.0f); }

static void plan606(int w, int h, uint32_t seed)
{
    size_t need = (size_t)w * h;
    if (pw606 != w || ph606 != h) {
        free(pl606); pl606 = (uint8_t *)malloc(need);
        pw606 = w; ph606 = h;
    }
    if (!pl606) return;
    memset(pl606, 0, need);
    row606 = 0;

    uint32_t s = seed ? seed : 0x51ED270Bu;
    int spot = (int)(f606(&s) * B606_NSPOT); if (spot >= B606_NSPOT) spot = B606_NSPOT - 1;
    double span = B606_SPOT[spot].span * (0.55 + 1.05 * (double)f606(&s));
    int    iters = 180 + (int)(f606(&s) * 420.0f);
    double rot   = (double)f606(&s) * 6.28318530718;
    int    band  = 3 + (int)(f606(&s) * 9.0f);
    int    base  = 8 + (int)(f606(&s) * 220.0f);

    par606[0] = B606_SPOT[spot].cx; par606[1] = B606_SPOT[spot].cy;
    par606[2] = span * 0.5;
    par606[3] = cos(rot); par606[4] = sin(rot);
    /* Scale so the ENTIRE escape range spans the 256-entry ramp about once.
     * Without this, a linear index (nu * band) overflows 256 several times on
     * a high-iteration view, and neighbouring pixels — whose escape counts
     * differ by one — land on unrelated colours. That reads as grey static,
     * not as detail. Compressing logarithmically also gives the low counts
     * (the wide-open areas) more of the ramp, which is where you want it. */
    par606[5] = (double)iters; par606[6] = (double)band; par606[7] = (double)base;
    par606[2] = span * 0.5;
}

static void slice606(int w, int h, int rows)
{
    if (!pl606 || row606 >= h) return;
    double cx = par606[0], cy = par606[1], half = par606[2];
    double cs = par606[3], sn = par606[4];
    int iters = (int)par606[5], band = (int)par606[6], base = (int)par606[7];
    double kscale = (180.0 + 12.0 * (double)band) / log((double)iters + 1.0);
    double aspect = (double)w / (double)h;
    int yend = row606 + rows; if (yend > h) yend = h;

    for (int y = row606; y < yend; y++) {
        double v = ((double)y / (double)h - 0.5) * 2.0 * half;
        uint8_t *row = pl606 + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            double u = ((double)x / (double)w - 0.5) * 2.0 * half * aspect;
            double pr = cx + (u * cs - v * sn);
            double pi = cy + (u * sn + v * cs);

            double zr = 0.0, zi = 0.0, zr2 = 0.0, zi2 = 0.0;
            double otrap = 1e30;
            int i = 0;
            while (i < iters && zr2 + zi2 <= 65536.0) {
                /* THE fold: abs() on both parts before squaring. Everything
                 * that makes this look like burning rigging comes from here. */
                double ar = fabs(zr), ai = fabs(zi);
                zi = 2.0 * ar * ai + pi;
                zr = ar * ar - ai * ai + pr;
                zr2 = zr * zr; zi2 = zi * zi;
                double od = zr2 + zi2;
                if (od < otrap) otrap = od;
                i++;
            }
            int idx;
            if (i >= iters) {
                /* Inside. The axis trap is CHAOTIC here — the abs() fold sends
                 * neighbouring pixels onto different cycles, so log(xtrap)
                 * jumps wildly and a steep slope turns the whole interior into
                 * grey static. The distance to the ORIGIN converges smoothly
                 * instead, and a gentle slope keeps it a gradient rather than
                 * noise. */
                double o = log(sqrt(otrap) + 1e-9);
                idx = base + 128 + (int)(o * -9.0);
            } else {
                /* Outside. NO trap term here. On the Mandelbrot the axis trap
                 * adds beautiful filaments; on the Burning Ship the abs() fold
                 * makes it chaotic, and it came out as grey static across the
                 * whole approach to the hull. The smooth escape count on its own
                 * is what gives this fractal its rigging. */
                double mag = sqrt(zr2 + zi2);
                double nu  = (double)i + 1.0 - log(log(mag) / log(2.0)) / log(2.0);
                if (nu < 0.0) nu = 0.0;
                idx = base + (int)(log(nu + 1.0) * kscale);
            }
            row[x] = (uint8_t)(idx & 255);
        }
    }
    row606 = yend;
}

void pattern_606(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (seed != seed606 || pw606 != w || ph606 != h) {
        plan606(w, h, seed);
        seed606 = seed;
    }
    if (!pl606) return;
    if (row606 < h) slice606(w, h, h / 45 + 1);

    uint32_t rot = (uint32_t)frame * 21u;
    uint32_t lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = pal[((uint32_t)i * 128u + rot) & JD_PAL_MASK];
    const uint8_t *src = pl606;
    size_t n = (size_t)w * h;
    for (size_t i = 0; i < n; i++) fb[i] = lut[src[i]];
}
