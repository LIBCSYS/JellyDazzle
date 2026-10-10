/* 632 Phoenix Fractal (fractal) — the set with a memory.
 *
 * Every other escape-time set in this library is first order: the next point
 * depends only on the current one.  The Phoenix map, Ushiki's, carries the
 * PREVIOUS iterate as well:
 *
 *     z[n+1] = z[n]^2 + p + q * z[n-1]
 *
 * That one extra term changes the character completely.  Instead of the
 * rotationally budding shapes the quadratic family produces, the attractor
 * grows a long body with swept feathered wings coming off it — which is why it
 * got the name.  Drawn with the axes swapped, as it is here, it stands upright
 * like a bird instead of lying on its side.
 *
 * p and q drift.  q is the one that matters: around -0.5 the wings are broad
 * and feathered, and as it moves toward zero the memory term weakens and the
 * whole thing collapses back toward an ordinary Julia set.  Keeping q in a
 * narrow loop near -0.5 keeps it a bird.  p shifts the body's proportions.
 * Both move slowly enough that it reads as the thing preening rather than as
 * an animation.
 *
 * COST STRATEGY.  Coarse grid plus rolling sweep, as 629/630/631.  Cells about
 * five canvas pixels, a band of fourteen grid rows per frame, canvas decay
 * holds the rest.  The second-order recurrence costs one extra multiply-add
 * pair per iteration over the plain quadratic, so the cap is kept at 36 — the
 * Phoenix escapes fast at this zoom and the extra iterations buy filigree too
 * fine for a five-pixel grid to resolve anyway.  Parameters advance once per
 * completed sweep, not per frame.
 */
#include "_spark572.h"

#define PX632_CELL 5.0f
#define PX632_BAND 14
#define PX632_IT   36

static gk       g632;
static uint32_t bs632 = 0xFFFFFFFFu;
static int      base632;
static int      row632;
static float    phi632;           /* parameter drift phase               */
static float    half632;
static float    qmid632;          /* centre of q's loop                  */
static float    pmid632;          /* centre of p's loop                  */

void pattern_632(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int gx, gy, i;
    gk_setup(&g632, w, h);
    gk_decay_snap(&g632, 0.90f);

    float cw = (float)g632.cw, ch = (float)g632.ch, sc = g632.sc;

    if (seed != bs632) {
        base632 = (int)(seed & 0x7FFFu);
        phi632  = gk_hash(seed + 41u) * GK_TAU;
        half632 = 1.25f + 0.40f * gk_hash(seed + 42u);
        /* q near -0.5: that is where the wings are.  Toward 0 the memory term
         * fades out and the bird turns back into a plain Julia blob. */
        qmid632 = -0.52f + 0.09f * (gk_hash(seed + 43u) - 0.5f);
        pmid632 =  0.5667f + 0.07f * (gk_hash(seed + 44u) - 0.5f);
        row632  = 0;
        gk_clear(&g632);
        bs632 = seed;
    }

    float cell  = PX632_CELL * sc;
    int   gcols = (int)(cw / cell) + 1;
    int   grows = (int)(ch / cell) + 1;

    float t  = (float)frame;
    float hh = half632 * (1.0f + 0.14f * sinf(phi632 * 0.61f));
    float pp = pmid632 + 0.055f * sinf(phi632);
    float qq = qmid632 + 0.040f * cosf(phi632 * 0.77f);
    float asp = cw / ch;

    float col[3];
    int   ylast = row632 + PX632_BAND;
    if (ylast > grows) ylast = grows;

    for (gy = row632; gy < ylast; gy++) {
        float py = ((float)gy + 0.5f) * cell;
        float sy = (py / ch - 0.5f) * 2.0f * hh;
        for (gx = 0; gx < gcols; gx++) {
            float px = ((float)gx + 0.5f) * cell;
            float sx = (px / cw - 0.5f) * 2.0f * hh * asp;

            /* AXES SWAPPED on purpose: screen y feeds the real part.  The
             * Phoenix's long axis is the real one, so without the swap the
             * bird lies sideways and the wings run off the left and right
             * edges of a landscape canvas. */
            float zr = sy, zi = sx;
            float wr = 0.0f, wi = 0.0f;         /* z[n-1], seeded at zero  */
            float zr2 = zr * zr, zi2 = zi * zi;
            i = 0;
            while (i < PX632_IT && zr2 + zi2 <= 16.0f) {
                float nr = zr2 - zi2 + pp + qq * wr;
                float ni = 2.0f * zr * zi      + qq * wi;
                wr = zr; wi = zi;               /* today becomes yesterday */
                zr = nr; zi = ni;
                zr2 = zr * zr; zi2 = zi * zi;
                i++;
            }

            float hue, amp;
            if (i >= PX632_IT) {
                /* Body of the bird.  Shade it by how far the final iterate
                 * sits from the origin rather than flat black — the interior
                 * of a Phoenix is not featureless and the shading shows the
                 * internal banding along the spine. */
                float m = sqrtf(zr2 + zi2);
                hue = 0.56f + m * 0.07f;
                amp = 0.30f + 0.22f * m;
            } else {
                float mag = sqrtf(zr2 + zi2);
                float nu  = (float)i + 1.0f
                          - logf(logf(mag) * 1.442695f) * 1.442695f;
                hue = nu * 0.038f;
                amp = 0.34f + 0.60f * (1.0f - (float)i / (float)PX632_IT);
            }
            sk_col(pal, sk_hidx(base632, hue + t * 0.00020f),
                   0.20f, 0.50f, amp, col);
            gk_dot(&g632, px, py, col, cell * 0.46f, cell * 0.66f, 0.28f);
        }
    }

    row632 = ylast;
    if (row632 >= grows) {
        row632 = 0;
        phi632 += 0.031f;                   /* full parameter loop ~34 s   */
        if (phi632 > GK_TAU) phi632 -= GK_TAU;
    }

    gk_present(&g632, fb, w, h);
}
