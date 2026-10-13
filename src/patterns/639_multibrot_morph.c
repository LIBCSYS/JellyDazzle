/* 639 Multibrot Morph (fractal) — the Mandelbrot set with its exponent let go.
 *
 * z -> z^d + c with d = 2 is the Mandelbrot set.  Raise d and the set grows
 * d-1 fold symmetry: three bulbs at d = 4, five at d = 6.  Here d is not an
 * integer that steps, it GLIDES — 2 to 6 and back on a slow sine — so the
 * picture blooms from the familiar one-lobed cardioid into a five-petalled
 * flower and folds back again, passing through every fractional power on
 * the way.  Non-integer powers use the polar form z^d = r^d (cos d0, sin d0),
 * which is what lets the petals grow in continuously instead of popping.
 *
 * Colour is the smooth escape count with the log taken in base d, so the
 * contour spacing stays even while the exponent changes under it.  The
 * interior is coloured by orbit trap (closest approach to zero), as 629, so
 * the middle of the flower is never a black hole.
 *
 * COST STRATEGY.  Coarse grid plus rolling sweep, as 629/632, with two
 * changes: the polar power costs an atan2, a log/exp pair and a sin/cos per
 * iteration, several times the plain quadratic, so the band is eight grid
 * rows and the cap 30.  d advances once per completed sweep, never mid-sweep,
 * and finished sweeps are crossfaded (_xfade.h), so the bloom is a flow.
 *
 * Seed: ramp start, where in the 2..6 cycle it opens, how fast it breathes,
 * the framing (rotation and a small off-centre drift) and hue density.
 */
#include "_spark572.h"
#include "_xfade.h"

#define MB639_CELL 5.0f           /* grid pitch, canvas px before scale   */
#define MB639_BAND 8              /* grid rows recomputed per frame       */
#define MB639_IT   30             /* iteration cap                        */

static gk       g639;
static gk_xf xf639;              /* crossfade of finished sweeps         */
static uint32_t bs639 = 0xFFFFFFFFu;
static int      base639;
static int      row639;           /* sweep position, in grid rows         */
static float    ph639;            /* phase of the 2..6 glide              */
static float    rate639;          /* phase step per completed sweep       */
static float    rot639;           /* view rotation                        */
static float    dens639;          /* hue per smoothed escape step         */
static float    off639[2];        /* off-centre drift amplitude           */

void pattern_639(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int gx, gy, i;
    gk_setup(&g639, w, h);
    if (row639 == 0) gk_clear(&g639);

    float cw = (float)g639.cw, ch = (float)g639.ch, sc = g639.sc;

    if (seed != bs639) {
        base639   = (int)(seed & 0x7FFFu);
        ph639     = gk_hash(seed + 21u) * GK_TAU;
        rate639   = 0.010f + 0.012f * gk_hash(seed + 22u);   /* ~25-55 s cycle */
        rot639    = gk_hash(seed + 23u) * GK_TAU;
        dens639   = 0.045f + 0.045f * gk_hash(seed + 24u);
        off639[0] = 0.18f * (gk_hash(seed + 25u) - 0.5f);
        off639[1] = 0.18f * (gk_hash(seed + 26u) - 0.5f);
        row639    = 0;
        gk_clear(&g639);
        gk_xf_reset(&xf639);
        bs639 = seed;
    }

    float cell  = MB639_CELL * sc;
    int   gcols = (int)(cw / cell) + 1;
    int   grows = (int)(ch / cell) + 1;

    /* The exponent: 4 +- 2 on a sine, eased at the ends so the cardioid and
     * the five-petal flower each get a moment to be seen. */
    float s  = sinf(ph639);
    float d  = 4.0f + 2.0f * s * (1.5f - 0.5f * s * s);
    float ld = 1.0f / logf(d);

    /* Higher powers make a smaller set; frame it so the flower fills the
     * screen at every d.  The view turns slowly the whole time. */
    float t   = (float)frame;
    float hh  = 1.00f - 0.06f * (d - 2.0f);
    /* d = 2 sits around -0.5 (the cardioid's body); higher powers are
     * centred on the origin.  Follow it so the set stays in frame. */
    float cx  = -0.55f * expf(-1.6f * (d - 2.0f));
    float ang = rot639 + t * 0.00028f;
    float cs  = cosf(ang), sn = sinf(ang);
    float asp = cw / ch;
    float ocr = off639[0] * sinf(t * 0.0011f) + cx;
    float oci = off639[1] * cosf(t * 0.0009f);

    float col[3];
    int   ylast = row639 + MB639_BAND;
    if (ylast > grows) ylast = grows;

    for (gy = row639; gy < ylast; gy++) {
        float py = ((float)gy + 0.5f) * cell;
        float v  = (py / ch - 0.5f) * 2.0f * hh;
        for (gx = 0; gx < gcols; gx++) {
            float px = ((float)gx + 0.5f) * cell;
            float u  = (px / cw - 0.5f) * 2.0f * hh * asp;

            float cr = u * cs - v * sn + ocr;
            float ci = u * sn + v * cs + oci;
            float zr = cr, zi = ci;            /* z1 = c: skip the zero step */
            float r2 = zr * zr + zi * zi;
            float trap = r2;
            i = 0;
            while (i < MB639_IT && r2 <= 64.0f) {
                /* z^d in polar form; r^d = exp(d/2 * ln r^2) */
                float a  = atan2f(zi, zr) * d;
                float rd = expf(0.5f * d * logf(r2 + 1e-30f));
                zr = rd * cosf(a) + cr;
                zi = rd * sinf(a) + ci;
                r2 = zr * zr + zi * zi;
                if (r2 < trap) trap = r2;
                i++;
            }

            float hue, amp, core, glow;
            if (i >= MB639_IT) {
                /* inside: a dim wash keyed to the orbit trap, so the bulbs
                 * show and the boundary is what glows */
                float lt = logf(trap + 1e-12f);
                hue  = 0.50f + lt * -0.030f;
                amp  = 0.26f;
                core = cell * 0.60f;
                glow = cell * 0.95f;
            } else {
                /* smooth count, log base d: nu = n + 1 - log_d(ln|z|) */
                float lz = 0.5f * logf(r2);
                float nu = (float)i + 1.0f - logf(lz > 1e-6f ? lz : 1e-6f) * ld;
                float k  = nu / (float)MB639_IT;
                if (k < 0.0f) k = 0.0f;
                hue  = nu * dens639;
                /* bright at the edge of the set, falling away into space */
                amp  = 0.10f + 0.85f * sqrtf(k);
                core = cell * 0.55f;
                glow = cell * 0.90f;
            }
            sk_col(pal, sk_hidx(base639, hue + t * 0.00020f),
                   0.22f, 0.50f, amp, col);
            gk_dot(&g639, px, py, col, core, glow, 0.30f);
        }
    }

    row639 = ylast;
    if (row639 >= grows) {                 /* sweep done: advance d, repeat */
        row639 = 0;
        gk_xf_commit(&xf639, &g639, (grows + MB639_BAND - 1) / MB639_BAND);
        ph639 += rate639;
        if (ph639 >= GK_TAU) ph639 -= GK_TAU;
    }

    gk_xf_present(&xf639, &g639, fb, w, h);
}
