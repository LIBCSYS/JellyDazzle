/* 631 Newton Basins (fractal) — territory, and the shimmer along its borders.
 *
 * Newton's method on the cubic z^3 - 1 steps z -> z - (z^3-1)/(3z^2) and from
 * almost anywhere lands on one of the three cube roots of unity.  Colour each
 * starting point by WHICH root it reached and the plane divides into three
 * territories whose borders are infinitely intricate: every point on a border
 * touches all three basins at once.  Cayley asked the question in 1879, which
 * makes this the oldest picture in the family, and it still looks like nothing
 * else here — interlocking bulbs, no filaments.
 *
 * What this one adds is the border itself.  The interesting pixels are the
 * slow ones: points that take many steps to commit to a root, which is exactly
 * the fractal seam.  Convergence speed drives brightness, and a travelling
 * noise field modulates that brightness, so the seams glitter and crawl while
 * the three territories sit almost still.  The relaxation constant in
 * z -> z - a·f/f' also drifts in a small loop around 1, which bends the
 * territories into each other and grows tendrils — slow enough that you only
 * notice it after watching for a while.
 *
 * COST STRATEGY.  Newton is the cheap one in this family: it converges in
 * fifteen to twenty steps, not hundreds.  Even so there is no full-resolution
 * per-pixel pass.  Grid cells are about five canvas pixels, a band of eighteen
 * grid rows is re-swept per frame (wider than 629/630 because each cell costs
 * less), and the canvas decay holds the rest.  The cubic is hard-coded rather
 * than general degree n, so z^3 and z^2 are three multiplies instead of a
 * loop.  Iteration cap 24, early exit when the step gets small.
 */
#include "_spark572.h"

#define NB631_CELL 5.0f
#define NB631_BAND 18
#define NB631_IT   24

static gk       g631;
static uint32_t bs631 = 0xFFFFFFFFu;
static int      base631;
static int      row631;
static float    phi631;           /* where the relaxation constant is    */
static float    half631;
static float    rot631;
static float    wand631;          /* how far `a` wanders from 1          */

void pattern_631(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int gx, gy, it;
    gk_setup(&g631, w, h);
    gk_decay_snap(&g631, 0.88f);

    float cw = (float)g631.cw, ch = (float)g631.ch, sc = g631.sc;

    if (seed != bs631) {
        base631 = (int)(seed & 0x7FFFu);
        phi631  = gk_hash(seed + 31u) * GK_TAU;
        half631 = 1.10f + 0.95f * gk_hash(seed + 32u);
        rot631  = gk_hash(seed + 33u) * GK_TAU;
        wand631 = 0.16f + 0.34f * gk_hash(seed + 34u);
        row631  = 0;
        gk_clear(&g631);
        bs631 = seed;
    }

    float cell  = NB631_CELL * sc;
    int   gcols = (int)(cw / cell) + 1;
    int   grows = (int)(ch / cell) + 1;

    float t  = (float)frame;
    float hh = half631 * (1.0f + 0.18f * sinf(phi631 * 0.73f));
    float cs = cosf(rot631), sn = sinf(rot631);
    float asp = cw / ch;

    /* a walks a small loop around 1.  At exactly 1 the basins are the clean
     * symmetric flower; off it they spiral into one another. */
    float ar = 1.0f + wand631 * cosf(phi631);
    float ai =        wand631 * sinf(phi631 * 1.31f);

    float col[3];
    int   ylast = row631 + NB631_BAND;
    if (ylast > grows) ylast = grows;

    for (gy = row631; gy < ylast; gy++) {
        float py = ((float)gy + 0.5f) * cell;
        float v  = (py / ch - 0.5f) * 2.0f * hh;
        for (gx = 0; gx < gcols; gx++) {
            float px = ((float)gx + 0.5f) * cell;
            float u  = (px / cw - 0.5f) * 2.0f * hh * asp;

            float zr = u * cs - v * sn;
            float zi = u * sn + v * cs;

            float last = 1.0f;
            it = 0;
            for (; it < NB631_IT; it++) {
                float zr2 = zr * zr, zi2 = zi * zi;
                float qr  = zr2 - zi2;             /* z^2                  */
                float qi  = 2.0f * zr * zi;
                float pr  = qr * zr - qi * zi - 1.0f;   /* z^3 - 1         */
                float pi  = qr * zi + qi * zr;
                float dr  = 3.0f * qr, di = 3.0f * qi;  /* 3 z^2           */
                float dd  = dr * dr + di * di;
                if (dd < 1e-16f) break;            /* sitting on the origin */

                float nr = (pr * dr + pi * di) / dd;
                float ni = (pi * dr - pr * di) / dd;
                float srp = ar * nr - ai * ni;     /* a * f/f'             */
                float sip = ar * ni + ai * nr;
                zr -= srp; zi -= sip;
                last = srp * srp + sip * sip;
                if (last < 1e-12f) { it++; break; }
            }

            /* which cube root did it commit to — angle 0, 2pi/3, 4pi/3 */
            float ang = atan2f(zi, zr);
            if (ang < 0.0f) ang += GK_TAU;
            int k = (int)(ang * (3.0f / GK_TAU) + 0.5f) % 3;

            /* Speed shades within the root's band.  The raw step count is a
             * small integer, so on its own each basin is a flat poster block;
             * folding in how far the LAST step travelled makes it continuous
             * and the basins get an interior gradient. */
            float sm = (float)it - logf(logf(last + 1e-30f) * -0.0345f + 1.001f);
            if (sm < 0.0f) sm = 0.0f;
            float frac = sm * (1.0f / (float)NB631_IT);
            if (frac > 1.0f) frac = 1.0f;

            /* SHIMMER.  Slow cells are the seam; a travelling noise field
             * modulates only those, so the territories hold still and their
             * borders glitter.  frac^3 keeps the modulation off the flat
             * interiors, which would otherwise read as crawling mush. */
            float edge = frac * frac * frac;
            float sh   = gk_noise1(px * 0.035f + py * 0.021f + t * 0.030f,
                                   700u + (uint32_t)k);
            float amp  = 0.26f + 0.62f * sqrtf(frac) + edge * (sh - 0.5f) * 1.5f;
            if (amp < 0.05f) amp = 0.05f;

            float hue = (float)k * 0.333f + frac * 0.11f + t * 0.00020f;
            sk_col(pal, sk_hidx(base631, hue), 0.18f, 0.48f, amp, col);
            gk_dot(&g631, px, py, col,
                   cell * (0.44f + 0.14f * edge), cell * 0.64f, 0.26f);
        }
    }

    row631 = ylast;
    if (row631 >= grows) {
        row631 = 0;
        phi631 += 0.052f;                  /* full loop of `a` about 20 s  */
        if (phi631 > GK_TAU) phi631 -= GK_TAU;
    }

    gk_present(&g631, fb, w, h);
}
