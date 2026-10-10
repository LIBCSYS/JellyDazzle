/* 630 Burning Ship Dive (fractal) — a slow descent into the armada.
 *
 * The Burning Ship is the Mandelbrot iteration with absolute values folded in:
 * z -> (|Re z| + i|Im z|)^2 + c.  Those two fabsf calls break the conformality
 * that makes the Mandelbrot set smooth, and what comes out instead is hard
 * horizontal structure — hulls, masts, rigging, a fleet of them repeating at
 * every scale.  It is the one set in this family that reads as architecture
 * rather than as foliage, which is the reason it is here.
 *
 * The motion is a dive, not a morph.  The view zooms toward one of a few
 * known-good coordinates deep in the fleet, holds nothing, and when the
 * iteration budget can no longer resolve the detail it fades and restarts from
 * the wide view at a different target.  Picking the target by hand matters:
 * zooming at a random point in this set lands in flat exterior within three
 * doublings, and what you get is an empty screen with a gradient on it.
 *
 * COST STRATEGY.  Coarse grid plus a rolling sweep, same discipline as 629.
 * Cells are about five canvas pixels; a band of twelve grid rows is recomputed
 * per frame and the canvas decay holds the rest, so a complete picture is on
 * screen while roughly 1/12th of it is being evaluated.  The iteration cap
 * starts at 28 and climbs with depth to a hard ceiling of 64 — deep zooms need
 * more iterations, but not unboundedly, because the reset is cheaper than the
 * iterations.  Zoom advances once per completed sweep so a band boundary reads
 * as a wipe rather than a tear.
 */
#include "_spark572.h"

#define BS630_CELL 5.0f
#define BS630_BAND 12
#define BS630_ITMIN 28
#define BS630_ITMAX 64

static gk       g630;
static uint32_t bs630 = 0xFFFFFFFFu;
static int      base630;
static int      row630;
static int      targ630;          /* which coordinate we are diving at   */
static float    half630;          /* current view half-height            */
static float    hue630;           /* hue offset for this dive            */

/* Hand-picked dive targets inside the fleet.  Each one stays interesting for
 * a long way down; a random point does not. */
static const float T630[5][2] = {
    { -1.7548f,  -0.0305f  },     /* the big ship, classic view          */
    { -1.77f,    -0.0325f  },     /* a smaller hull off its bow          */
    { -1.9407f,  -0.0007f  },     /* long thin masts                     */
    { -1.5807f,  -0.0353f  },     /* dense rigging, high contrast        */
    { -1.86f,    -0.0105f  }      /* repeating fleet, good at depth      */
};

#define BS630_H0  1.30f           /* starting half-height (wide)         */
#define BS630_HMIN 7.0e-5f        /* bottom of the dive                  */

void pattern_630(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int gx, gy, i;
    gk_setup(&g630, w, h);
    gk_decay_snap(&g630, 0.90f);

    float cw = (float)g630.cw, ch = (float)g630.ch, sc = g630.sc;

    if (seed != bs630) {
        base630 = (int)(seed & 0x7FFFu);
        targ630 = (int)(gk_hash(seed + 21u) * 5.0f) % 5;
        half630 = BS630_H0;
        hue630  = gk_hash(seed + 22u);
        row630  = 0;
        gk_clear(&g630);
        bs630 = seed;
    }

    float cell  = BS630_CELL * sc;
    int   gcols = (int)(cw / cell) + 1;
    int   grows = (int)(ch / cell) + 1;

    float tr = T630[targ630][0], ti = T630[targ630][1];

    /* Iterations track depth.  logf of the zoom factor is the number of
     * doublings in, and each doubling costs a few more iterations before the
     * boundary stops resolving. */
    float depth = logf(BS630_H0 / half630) * 1.442695f;      /* in doublings */
    int   iters = BS630_ITMIN + (int)(depth * 2.6f);
    if (iters > BS630_ITMAX) iters = BS630_ITMAX;

    float t   = (float)frame;
    float asp = cw / ch;
    float col[3];

    /* Fade the picture out near the bottom of the dive so the reset to the
     * wide view is a dissolve instead of a cut. */
    float close = half630 / BS630_HMIN;                      /* 1 at bottom  */
    float gate  = close < 3.0f ? (close - 1.0f) * 0.5f : 1.0f;
    if (gate < 0.0f) gate = 0.0f;

    int ylast = row630 + BS630_BAND;
    if (ylast > grows) ylast = grows;

    for (gy = row630; gy < ylast; gy++) {
        float py = ((float)gy + 0.5f) * cell;
        float ci = ti + (py / ch - 0.5f) * 2.0f * half630;
        for (gx = 0; gx < gcols; gx++) {
            float px = ((float)gx + 0.5f) * cell;
            float cr = tr + (px / cw - 0.5f) * 2.0f * half630 * asp;

            float zr = 0.0f, zi = 0.0f;
            float zr2 = 0.0f, zi2 = 0.0f;
            i = 0;
            while (i < iters && zr2 + zi2 <= 16.0f) {
                /* the fold: square the componentwise absolute value */
                float ar = fabsf(zr), ai = fabsf(zi);
                zi = 2.0f * ar * ai + ci;
                zr = zr2 - zi2 + cr;
                zr2 = zr * zr; zi2 = zi * zi;
                i++;
            }

            float hue, amp;
            if (i >= iters) {
                /* Interior: one dark band of its own rather than black, so the
                 * hulls still read as solid objects with a surface. */
                hue = hue630 + 0.52f;
                amp = 0.20f;
            } else {
                float mag = sqrtf(zr2 + zi2);
                float nu  = (float)i + 1.0f
                          - logf(logf(mag) * 1.442695f) * 1.442695f;
                /* Divide by the iteration cap before scaling hue.  Without it
                 * the hue sweep per screen grows as the cap climbs with depth,
                 * and the colour scheme visibly changes as you descend. */
                hue = hue630 + nu * (1.9f / (float)iters);
                amp = 0.40f + 0.58f * (1.0f - (float)i / (float)iters);
            }
            sk_col(pal, sk_hidx(base630, hue + t * 0.00018f),
                   0.20f, 0.52f, amp * gate, col);
            gk_dot(&g630, px, py, col, cell * 0.46f, cell * 0.66f, 0.28f);
        }
    }

    row630 = ylast;
    if (row630 >= grows) {
        row630 = 0;
        half630 *= 0.972f;                /* about 28 s from wide to bottom */
        if (half630 < BS630_HMIN) {       /* surface and pick a new ship    */
            half630 = BS630_H0;
            targ630 = (targ630 + 1 + (int)(gk_hash((uint32_t)frame) * 3.0f)) % 5;
            hue630  = gk_hash((uint32_t)frame ^ 0x5A5Au);
        }
    }

    gk_present(&g630, fb, w, h);
}
