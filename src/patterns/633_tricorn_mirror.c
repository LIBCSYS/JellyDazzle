/* 633 Tricorn Mirror (fractal) — the Mandelbrot's conjugate twin.
 *
 * Put a conjugate in the iteration — z -> conj(z)^2 + c — and the Mandelbrot
 * set turns into the Tricorn, or Mandelbar.  One sign flip, and the two-fold
 * budding symmetry of the Mandelbrot becomes THREE-fold: a central body with
 * three horns, each horn carrying its own smaller copy.  The conjugate also
 * destroys conformality, so where the Mandelbrot has smooth round bulbs the
 * Tricorn has cusps and straight-ish spines.  It is the cheapest interesting
 * variation on the quadratic family: the inner loop is identical to the
 * Mandelbrot's but for one minus sign.
 *
 * MIRROR, and why the framing is locked.  The Tricorn is exactly symmetric
 * about the real axis, so only the upper half plane is ever evaluated and each
 * result is stamped twice — once at its own row, once at the row mirrored
 * through the canvas centre line.  That halves the cost outright, and it is
 * the reason this pattern does NOT rotate the view the way 629 and 631 do:
 * a rotated view breaks the axis alignment and the mirror stops being
 * correct.  The framing therefore only zooms and pans horizontally, which
 * keeps the real axis on the centre line where the symmetry lives.
 *
 * COST STRATEGY.  Coarse grid, five-pixel cells, rolling sweep — but the sweep
 * only has half as many rows to cover, so a band of ten grid rows per frame
 * refreshes the whole frame faster than 629's fourteen.  Iteration cap 44,
 * early bail.  Effective cost is roughly half of a comparable full-plane
 * escape-time pattern at the same grid pitch.
 */
#include "_spark572.h"

#define TC633_CELL 5.0f
#define TC633_BAND 10             /* grid rows per frame, upper half only */
#define TC633_IT   44

static gk       g633;
static uint32_t bs633 = 0xFFFFFFFFu;
static int      base633;
static int      row633;
static float    phi633;
static float    half633;
static float    pan633;           /* horizontal pan centre               */

void pattern_633(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int gx, gy, i;
    gk_setup(&g633, w, h);
    gk_decay_snap(&g633, 0.90f);

    float cw = (float)g633.cw, ch = (float)g633.ch, sc = g633.sc;

    if (seed != bs633) {
        base633 = (int)(seed & 0x7FFFu);
        phi633  = gk_hash(seed + 51u) * GK_TAU;
        half633 = 1.05f + 0.55f * gk_hash(seed + 52u);
        pan633  = -0.35f + 0.30f * (gk_hash(seed + 53u) - 0.5f);
        row633  = 0;
        gk_clear(&g633);
        bs633 = seed;
    }

    float cell  = TC633_CELL * sc;
    int   gcols = (int)(cw / cell) + 1;
    int   ghalf = (int)(ch * 0.5f / cell) + 2;   /* rows from centre out   */

    float t   = (float)frame;
    float hh  = half633 * (1.0f + 0.20f * sinf(phi633));
    float pan = pan633 + 0.18f * sinf(phi633 * 0.47f);
    float asp = cw / ch;
    float mid = ch * 0.5f;

    float col[3];
    int   ylast = row633 + TC633_BAND;
    if (ylast > ghalf) ylast = ghalf;

    for (gy = row633; gy < ylast; gy++) {
        float dy = ((float)gy + 0.5f) * cell;   /* distance above centre   */
        float ptop = mid - dy, pbot = mid + dy;
        float ci = (dy / ch) * 2.0f * hh;       /* imaginary part, >= 0    */
        for (gx = 0; gx < gcols; gx++) {
            float px = ((float)gx + 0.5f) * cell;
            float cr = pan + (px / cw - 0.5f) * 2.0f * hh * asp;

            float zr = 0.0f, zi = 0.0f;
            float zr2 = 0.0f, zi2 = 0.0f;
            i = 0;
            while (i < TC633_IT && zr2 + zi2 <= 16.0f) {
                /* the only difference from the Mandelbrot: the sign here */
                zi = -2.0f * zr * zi + ci;
                zr = zr2 - zi2 + cr;
                zr2 = zr * zr; zi2 = zi * zi;
                i++;
            }

            float hue, amp;
            if (i >= TC633_IT) {
                hue = 0.60f;
                amp = 0.18f;
            } else {
                float mag = sqrtf(zr2 + zi2);
                float nu  = (float)i + 1.0f
                          - logf(logf(mag) * 1.442695f) * 1.442695f;
                hue = nu * 0.034f;
                /* Trimmed relative to the other escape-time patterns here:
                 * every cell is stamped twice, so the same amp reads hotter
                 * than it does in 629 or 632. */
                amp = 0.31f + 0.50f * (1.0f - (float)i / (float)TC633_IT);
            }
            sk_col(pal, sk_hidx(base633, hue + t * 0.00021f),
                   0.20f, 0.50f, amp, col);

            /* one evaluation, two stamps — this is the whole saving */
            gk_dot(&g633, px, ptop, col, cell * 0.46f, cell * 0.66f, 0.28f);
            if (dy > cell * 0.25f)
                gk_dot(&g633, px, pbot, col, cell * 0.46f, cell * 0.66f, 0.28f);
        }
    }

    row633 = ylast;
    if (row633 >= ghalf) {
        row633 = 0;
        phi633 += 0.019f;                  /* zoom/pan loop about 55 s    */
        if (phi633 > GK_TAU) phi633 -= GK_TAU;
    }

    gk_present(&g633, fb, w, h);
}
