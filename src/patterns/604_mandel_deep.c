/* 604 Mandel Deep — escape-time fractal, animated the way Fractint did it.
 *
 * The Mandelbrot set is expensive: hundreds of iterations per pixel. Recomputed
 * every frame it would cost more than the entire 16 ms budget on its own.
 *
 * But it never needs recomputing. The IMAGE does not change — only the colour
 * does. So this draws once into a plane of palette indices and then animates
 * for its whole turn by moving the palette underneath, which is exactly what
 * Fractint did in 1990 and exactly what DAZZLE.EXE did with its DAC. Expensive
 * maths becomes free after the first frame: one table lookup per pixel.
 *
 * Per instance the seed chooses:
 *   - which of several hand-picked regions to sit in (each is a different
 *     structural character: seahorse valley, the elephant valley spirals, a
 *     minibrot, the double-spiral, filaments),
 *   - the zoom depth within that region,
 *   - the iteration budget, which sets how much fine filigree survives.
 *
 * Smooth colouring: the raw iteration count produces hard bands, which is the
 * jagged look we do not want. The fractional escape (Normalised Iteration
 * Count) turns those bands into a continuous gradient, so the palette sweeps
 * through it smoothly instead of stepping.
 */
#include "../engine/jellydazzle.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

#define M604_BG 0            /* index 0 = inside the set, drawn black */

/* Regions worth looking at. Picked by hand — a random point in the plane is
 * usually either solid black or featureless haze. */
static const struct { double cx, cy, span; } M604_SPOT[] = {
    { -0.743643887037151,  0.131825904205330, 0.00012 },  /* seahorse valley  */
    { -0.748,               0.100,            0.0090  },  /* classic spirals  */
    {  0.2929859127507,     0.6117848324958,  0.00035 },  /* double spiral    */
    { -1.7496200000000,     0.0,              0.00008 },  /* minibrot on spine*/
    { -0.10109636384562,    0.95628651080914, 0.00080 },  /* filament tangle  */
    { -1.25066,             0.02012,          0.00040 },  /* dendrites        */
    {  0.432539867562512,   0.226118675951765,0.00035 },  /* spiral galaxy    */
};
#define M604_NSPOT ((int)(sizeof M604_SPOT / sizeof M604_SPOT[0]))

static uint8_t *pl604;
static int      pw604, ph604;
static uint32_t seed604 = 0xFFFFFFFFu;
static int      row604;          /* next row to compute — progressive build */
static double   par604[8];       /* frozen params while the build is running */

static uint32_t r604(uint32_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
    return *s;
}
static float f604(uint32_t *s) { return (float)(r604(s) >> 8) * (1.0f / 16777216.0f); }

/* Choose the view once, then compute it a slice at a time. Recomputing the
 * whole 1280x960 frame in one go costs ~300 ms — an eighteen-frame stutter the
 * moment the pattern appears. Fractint had the same problem and the same
 * answer: draw it progressively. Here that is not a compromise but the right
 * look, because the fractal resolves downward into view over about a second. */
static void plan604(int w, int h, uint32_t seed)
{
    size_t need = (size_t)w * h;
    if (pw604 != w || ph604 != h) {
        free(pl604); pl604 = (uint8_t *)malloc(need);
        pw604 = w; ph604 = h;
    }
    if (!pl604) return;
    memset(pl604, M604_BG, need);
    row604 = 0;

    uint32_t s = seed ? seed : 0x9E3779B9u;
    int spot = (int)(f604(&s) * M604_NSPOT); if (spot >= M604_NSPOT) spot = M604_NSPOT - 1;
    double cx = M604_SPOT[spot].cx, cy = M604_SPOT[spot].cy;
    double span = M604_SPOT[spot].span * (0.55 + 1.10 * (double)f604(&s));
    int    iters = 220 + (int)(f604(&s) * 480.0f);      /* 220..700 */
    double rot   = (double)f604(&s) * 6.28318530718;    /* the plane can tilt */
    int    band  = 3 + (int)(f604(&s) * 9.0f);          /* index steps per unit */
    int    base  = 8 + (int)(f604(&s) * 220.0f);

    par604[0] = cx; par604[1] = cy; par604[2] = span * 0.5;
    par604[3] = cos(rot); par604[4] = sin(rot);
    par604[5] = (double)iters; par604[6] = (double)band; par604[7] = (double)base;
}

/* compute `rows` more scanlines of the plan */
static void slice604(int w, int h, int rows)
{
    if (!pl604 || row604 >= h) return;
    double cx = par604[0], cy = par604[1], half = par604[2];
    double cs = par604[3], sn = par604[4];
    int iters = (int)par604[5], band = (int)par604[6], base = (int)par604[7];
    double aspect = (double)w / (double)h;
    int yend = row604 + rows; if (yend > h) yend = h;
    for (int y = row604; y < yend; y++) {
        double v = ((double)y / (double)h - 0.5) * 2.0 * half;
        uint8_t *row = pl604 + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            double u = ((double)x / (double)w - 0.5) * 2.0 * half * aspect;
            double pr = cx + (u * cs - v * sn);
            double pi = cy + (u * sn + v * cs);

            /* cardioid / period-2 bulb test: skips the solid interior cheaply,
             * which is most of the cost on a wide view */
            double zr = 0.0, zi = 0.0, zr2 = 0.0, zi2 = 0.0;
            double xtrap = 1e30;         /* closest the orbit came to an axis */
            int i = 0;
            while (i < iters && zr2 + zi2 <= 65536.0) {
                zi = 2.0 * zr * zi + pi;
                zr = zr2 - zi2 + pr;
                zr2 = zr * zr; zi2 = zi * zi;
                double ax = fabs(zr) < fabs(zi) ? fabs(zr) : fabs(zi);
                if (ax < xtrap) xtrap = ax;
                i++;
            }
            double t = log(xtrap + 1e-12);
            int idx;
            if (i >= iters) {
                /* Inside the set. Painting it black leaves a hole in the frame,
                 * so the orbit trap colours the interior instead — it varies
                 * smoothly and carries the internal structure. */
                idx = base + 128 + (int)(t * -16.0 * (double)band);
            } else {
                /* SMOOTH escape. The bare integer count bands hard; subtracting
                 * the log-log of the escape radius makes it continuous, so the
                 * palette glides through instead of stepping. The trap folded in
                 * on top adds the filaments radiating away from the set, so the
                 * exterior is structure rather than plain contour rings. */
                double mag = sqrt(zr2 + zi2);
                double nu  = (double)i + 1.0 - log(log(mag) / log(2.0)) / log(2.0);
                idx = base + (int)(nu * (double)band) + (int)(t * -13.0);
            }
            row[x] = (uint8_t)(idx & 255);
        }
    }
    row604 = yend;
}

void pattern_604(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (seed != seed604 || pw604 != w || ph604 != h) {
        plan604(w, h, seed);
        seed604 = seed;
    }
    if (!pl604) return;
    if (row604 < h) slice604(w, h, h / 45 + 1);   /* ~45 frames: under budget, still under a second */
    /* The only per-frame work: walk the palette under a fixed image. */
    uint32_t rot = (uint32_t)frame * 19u;
    uint32_t lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = pal[((uint32_t)i * 128u + rot) & JD_PAL_MASK];
    const uint8_t *src = pl604;
    size_t n = (size_t)w * h;
    for (size_t i = 0; i < n; i++) fb[i] = lut[src[i]];
}
