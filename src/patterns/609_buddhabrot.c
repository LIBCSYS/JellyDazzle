/* 609 Buddhabrot — the Mandelbrot's ghost, developed like a photograph.
 *
 * Every other escape-time routine here asks a question about the PIXEL: how
 * long did the point at this location take to run away? Melinda Green's 1993
 * trick asks about the PATH instead. Fire a point that is known to escape, and
 * record every place its orbit visited on the way out. Do that for millions of
 * points and the accumulated exposure map is not the familiar black cardioid at
 * all — it is a luminous, translucent figure that looks like a seated Buddha,
 * built entirely out of trajectories that were on their way to infinity.
 *
 * It cannot be computed pixel by pixel. It has to be EXPOSED — which is the
 * reason to have it here. Every other routine in the library arrives complete;
 * this one arrives over about a minute, faint at first, gathering density and
 * detail the longer it is left alone, the way a plate develops. Nothing else in
 * JellyDazzle does that, and it means the routine looks different at second ten
 * than it does at second sixty.
 *
 * Practicalities:
 *   - a fixed budget of trajectories is fired per frame, so the cost is flat
 *     and predictable no matter how long the exposure has been running;
 *   - only escaping orbits are recorded (the interior contributes nothing,
 *     which is exactly why the figure is hollow);
 *   - density is mapped logarithmically, because the head and shoulders are
 *     hundreds of times denser than the aura and a linear map would show one
 *     or the other but never both.
 *
 * Colour: density picks the index, so the exposure map IS the palette index
 * plane, and the palette then rotates through it like everything else. Even
 * zero density lands on a live colour — nothing is painted black.
 */
#include "../engine/jellydazzle.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

#define B609_SHOTS 5200           /* trajectories fired per frame */
#define B609_MAXIT 260            /* orbit length considered                 */
#define B609_MINIT 12             /* below this the orbit is a boring stub   */

static uint32_t *cnt609;          /* the exposure map */
static int       pw609, ph609;
static uint32_t  seed609 = 0xFFFFFFFFu;
static uint32_t  rs609;
static uint32_t  peak609;         /* running maximum, for the log scale */
static double    par609[8];

static uint32_t r609(uint32_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
    return *s;
}
static double d609(uint32_t *s) { return (double)(r609(s) >> 8) * (1.0 / 16777216.0); }

static void plan609(int w, int h, uint32_t seed)
{
    size_t need = (size_t)w * h * sizeof(uint32_t);
    if (pw609 != w || ph609 != h) {
        free(cnt609); cnt609 = (uint32_t *)malloc(need);
        pw609 = w; ph609 = h;
    }
    if (!cnt609) return;
    memset(cnt609, 0, need);
    peak609 = 1;

    uint32_t s = seed ? seed : 0x8DDA1993u;
    rs609 = s | 1u;
    par609[0] = 8.0 + d609(&s) * 220.0;          /* palette base              */
    par609[1] = 0.55 + d609(&s) * 0.85;          /* view scale                */
    par609[2] = (d609(&s) - 0.5) * 0.55;         /* centre drift, real        */
    par609[3] = (d609(&s) - 0.5) * 0.55;         /* centre drift, imaginary   */
    par609[4] = d609(&s) < 0.5 ? 0.0 : 1.0;      /* upright or on its side    */
    par609[5] = 70.0 + d609(&s) * 130.0;         /* density contrast          */
}

/* Fire a batch of trajectories and record where they went. */
static void expose609(int w, int h)
{
    if (!cnt609) return;
    double scale = par609[1];
    double dr = par609[2], di = par609[3];
    int    side = par609[4] > 0.5;

    /* the plane window, sized so the whole figure fits the short axis */
    double halfy = 1.55 * scale;
    double halfx = halfy * (double)w / (double)h;

    double zr_hist[B609_MAXIT], zi_hist[B609_MAXIT];

    for (int shot = 0; shot < B609_SHOTS; shot++) {
        /* Sample over the region that actually contributes. Points far outside
         * escape in two steps and contribute nothing but noise. */
        double cr = -2.15 + d609(&rs609) * 3.05;
        double ci = -1.30 + d609(&rs609) * 2.60;

        /* cheap interior rejection: the cardioid and the period-2 bulb never
         * escape, so testing them is wasted work */
        double q = (cr - 0.25) * (cr - 0.25) + ci * ci;
        if (q * (q + (cr - 0.25)) <= 0.25 * ci * ci) continue;
        if ((cr + 1.0) * (cr + 1.0) + ci * ci <= 0.0625) continue;

        double zr = 0.0, zi = 0.0, zr2 = 0.0, zi2 = 0.0;
        int n = 0;
        while (n < B609_MAXIT && zr2 + zi2 <= 16.0) {
            zi = 2.0 * zr * zi + ci;
            zr = zr2 - zi2 + cr;
            zr2 = zr * zr; zi2 = zi * zi;
            zr_hist[n] = zr; zi_hist[n] = zi;
            n++;
        }
        if (n >= B609_MAXIT) continue;      /* never escaped: contributes nothing */
        if (n < B609_MINIT)  continue;      /* escaped instantly: just haze       */

        /* Replay the orbit onto the exposure map. This is the whole trick —
         * the picture is made of paths, not of destinations. */
        for (int k = 0; k < n; k++) {
            double px = zr_hist[k] + 0.45 + dr;
            double py = zi_hist[k] + di;
            double sx = side ? py : px;
            double sy = side ? px : py;
            int ix = (int)((sx / halfx * 0.5 + 0.5) * (double)w);
            int iy = (int)((sy / halfy * 0.5 + 0.5) * (double)h);
            if (ix < 0 || ix >= w || iy < 0 || iy >= h) continue;
            uint32_t c = ++cnt609[(size_t)iy * w + ix];
            if (c > peak609) peak609 = c;
        }
    }
}

void pattern_609(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (seed != seed609 || pw609 != w || ph609 != h) {
        plan609(w, h, seed);
        seed609 = seed;
    }
    if (!cnt609) return;
    expose609(w, h);

    int    base  = (int)par609[0];
    double contr = par609[5];
    /* Logarithmic density. The dense core is orders of magnitude brighter than
     * the aura; a linear map shows one or the other and never both. */
    double norm = contr / log((double)peak609 + 1.0);

    uint32_t rot = (uint32_t)frame * 11u;
    uint32_t lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = pal[((uint32_t)i * 128u + rot) & JD_PAL_MASK];

    const uint32_t *src = cnt609;
    size_t n = (size_t)w * h;
    for (size_t i = 0; i < n; i++) {
        int idx = base + (int)(log((double)src[i] + 1.0) * norm);
        fb[i] = lut[idx & 255];
    }
}
