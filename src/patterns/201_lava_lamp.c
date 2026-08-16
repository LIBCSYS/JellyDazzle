/* pattern_201 — LAVA LAMP (overlay class)
 *
 * Seven metaballs drift on independent slow clocks, merging and parting the
 * way wax does: nothing bounces, nothing snaps, and no two blobs share a
 * period so the arrangement never repeats.  Each blob carries its own colour,
 * and where two blobs meet their COLOURS are mixed — see the note on the
 * accumulators below, which is the whole reason this pattern looks like wax
 * and not like a rainbow decal.
 *
 * Everything outside the wax is BLACK on purpose: the compositor blends this
 * with MAX/screen, so black means "let the layer underneath through".  That
 * is what makes it an overlay rather than a wall.
 *
 * Cost: the scalar field is accumulated at 1/4 resolution (the field is very
 * smooth, so quarter-res loses nothing) inside per-blob bounding boxes, then
 * bilinearly upsampled — field AND colour both — while it is written out.
 */
#include "../engine/jellydazzle.h"
#include <string.h>
#include <stdlib.h>

#define P201_DIV   4                     /* field resolution divisor        */
#define P201_NB    7                     /* blobs                           */
#define P201_MAXW  512                   /* field buffer capacity (w/DIV)   */
#define P201_MAXH  384
#define P201_SLOW  3                      /* period multiplier: wax is slow  */
#define P201_THR   200                    /* field value at the wax surface  */
#define P201_SHLD  220                    /* shoulder width above threshold  */

static uint16_t p201_field[P201_MAXW * P201_MAXH];
/* Colour is accumulated as three weighted CHANNEL sums, not as a weighted
 * palette INDEX.  Averaging indices was the old way and it is wrong: two
 * blobs 12000 palette entries apart average to entry 6000, which is some
 * unrelated third hue, so every seam drew a thin bright stripe of whatever
 * happened to live halfway round the ramp.  Mixing R/G/B instead means a
 * seam can only ever contain a blend of the two colours actually meeting
 * there. */
static uint32_t p201_rs[P201_MAXW * P201_MAXH];
static uint32_t p201_gs[P201_MAXW * P201_MAXH];
static uint32_t p201_bs[P201_MAXW * P201_MAXH];
static uint32_t p201_col[P201_MAXW * P201_MAXH];   /* normalised, packed RGB */

/* Q14 sine, 256 entries — self-contained so the pattern never depends on
 * engine tables. */
static int32_t p201_sin[256];
static int     p201_ready;

static void p201_init(void)
{
    for (int i = 0; i < 256; i++) {
        double a = (double)i * 6.283185307179586 / 256.0;
        /* no libm dependency: Taylor around the reduced angle */
        double x = a > 3.14159265 ? a - 6.28318531 : a;
        double x2 = x * x, s = x * (1.0 - x2 / 6.0 * (1.0 - x2 / 20.0
                      * (1.0 - x2 / 42.0 * (1.0 - x2 / 72.0))));
        p201_sin[i] = (int32_t)(s * 16384.0);
    }
    p201_ready = 1;
}

/* phase is Q16: 65536 units = one full cycle */
static inline int32_t p201_s(int32_t phase)
{
    uint32_t p = (uint32_t)phase;
    int i = (p >> 8) & 255, f = p & 255;
    int32_t a = p201_sin[i], b = p201_sin[(i + 1) & 255];
    return a + (((b - a) * f) >> 8);
}

/* packed-RGB lerp, w in 0..256.  Each channel sits in its own 16-bit lane
 * after the multiply, so the lanes cannot bleed into one another. */
static inline uint32_t p201_mix(uint32_t a, uint32_t b, uint32_t w)
{
    uint32_t iw = 256u - w;
    uint32_t rb = (((a & 0x00FF00FFu) * iw + (b & 0x00FF00FFu) * w) >> 8) & 0x00FF00FFu;
    uint32_t g  = (((a & 0x0000FF00u) * iw + (b & 0x0000FF00u) * w) >> 8) & 0x0000FF00u;
    return rb | g;
}

void pattern_201(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (!p201_ready) p201_init();

    int fw = w / P201_DIV, fh = h / P201_DIV;
    if (fw > P201_MAXW) fw = P201_MAXW;
    if (fh > P201_MAXH) fh = P201_MAXH;
    if (fw < 8 || fh < 8) { memset(fb, 0, (size_t)w * h * 4); return; }

    size_t ncell = (size_t)fw * fh;
    memset(p201_field, 0, ncell * sizeof *p201_field);
    memset(p201_rs,    0, ncell * sizeof *p201_rs);
    memset(p201_gs,    0, ncell * sizeof *p201_gs);
    memset(p201_bs,    0, ncell * sizeof *p201_bs);

    /* ---- place the wax -------------------------------------------------
     * Vertical travel is slow and eased (a lamp's wax hangs at the top and
     * bottom); horizontal sway is slower still and half the amplitude, so
     * blobs wander rather than orbit. Periods are mutually prime multiples
     * of a base tick so the set never re-forms the same arrangement.
     * Phase is Q16 — 65536 units per cycle — so per_y[b]*P201_SLOW frames
     * is one full rise-and-fall: ~49 s at 60 fps for the fastest blob. */
    static const int per_y[P201_NB] = { 977, 1213, 1471, 1747, 2069, 2371, 2707 };
    static const int per_x[P201_NB] = { 1601, 1877, 2153, 2447, 2749, 3067, 3391 };

    uint32_t flow = (uint32_t)frame * 3u;

    for (int b = 0; b < P201_NB; b++) {
        uint32_t hs = seed * 2654435761u + (uint32_t)b * 0x9E3779B1u;
        hs ^= hs >> 15;

        /* Spread the blobs around the vertical cycle.  The stagger must NOT
         * run monotonically with b, because the periods do: a spread of
         * b*65536/NB grows with b while the frame term 65536*frame/per_y[b]
         * shrinks with b, so the two cancel and every phase collapses into
         * one quadrant — measured at frame 4000, all seven blobs landed
         * within 0.19..0.37 of a cycle and drew a single horizontal bar.
         * The golden-ratio sequence spreads them just as evenly but in an
         * order uncorrelated with period, so nothing can cancel. */
        int32_t spread = (int32_t)(((uint32_t)b * 0x9E3779B1u) >> 16);
        int32_t py = (int32_t)(((int64_t)frame << 16) / (per_y[b] * P201_SLOW))
                   + spread + (int32_t)(hs & 8191);
        int32_t px = (int32_t)(((int64_t)frame << 16) / (per_x[b] * P201_SLOW))
                   + (int32_t)((hs >> 8) & 65535);

        /* ease the vertical sine so it lingers at the extremes: s - s^3/3 */
        int32_t sy = p201_s(py);
        int32_t cube = (int32_t)(((int64_t)sy * sy >> 14) * sy >> 14);
        sy = sy - cube / 3;

        /* Everything below is Q8 in FIELD pixels.  Sub-cell precision is not
         * a nicety: the field grid is quarter-res, so an integer centre made
         * each blob sit still for ~5 frames and then teleport 4 screen pixels
         * — measured as a frame-to-frame delta alternating 0.008 / 0.52,
         * which is precisely the stepping the eye reads as jerk. */
        int home8 = (fw * (b * 2 + 1) * 256) / (P201_NB * 2);
        int cx8 = home8 + (int)(((int64_t)p201_s(px) * (fw * 256 / 6)) >> 15);

        /* Each blob also gets its OWN vertical span — a different mid-height
         * and a different travel.  Staggering the phase alone is not enough:
         * the eased sine makes every blob linger at its extremes, so a set of
         * blobs sharing one amplitude spends much of its time strung out
         * along a single horizontal line, which is the one arrangement a
         * lamp never makes. */
        uint32_t hs2 = hs * 0x85EBCA6Bu; hs2 ^= hs2 >> 13;
        int amp8 = (fh * 2 * 256 / 5) * (int)(168u + (hs2 & 87u)) >> 8;
        int mid8 = fh * 128 + ((int)((hs2 >> 9) & 31) - 16) * fh * 2;
        int cy8  = mid8 + (int)(((int64_t)sy * amp8) >> 14);

        /* radius breathes gently, each blob on its own clock */
        int rbase8 = (fh * (5 + (int)((hs >> 16) & 3)) * 256) / 52;
        int r8 = rbase8 + (int)(((int64_t)p201_s(py * 2 + 9000) * (rbase8 / 5)) >> 14);
        if (r8 < 1024) r8 = 1024;

        /* ---- this blob's colour, resolved ONCE per frame ---------------
         * Blobs are spread right across the palette so a lamp shows several
         * distinct hues at once.  A blob that lands on a near-black palette
         * entry is a dead lump on a black field, so lift anything dim until
         * it actually emits: the overlay's job is to add light. */
        uint32_t tint = ((uint32_t)b * (32768u / P201_NB) + ((hs >> 18) & 0x7FFu)) & 0x7FFFu;
        uint32_t c = pal[(tint + flow) & JD_PAL_MASK];
        int cr = (int)((c >> 16) & 255), cg = (int)((c >> 8) & 255), cb = (int)(c & 255);
        int lum = (cr * 77 + cg * 150 + cb * 29) >> 8;
        if (lum < 78) {
            int gain = (78 * 256) / (lum > 0 ? lum : 1);
            if (gain > 1024) gain = 1024;
            cr = cr * gain >> 8; if (cr > 255) cr = 255;
            cg = cg * gain >> 8; if (cg > 255) cg = 255;
            cb = cb * gain >> 8; if (cb > 255) cb = 255;
            lum = (cr * 77 + cg * 150 + cb * 29) >> 8;
            if (lum < 56) {                       /* was ~pure black: grey it up */
                int add = 56 - lum;
                cr += add; if (cr > 255) cr = 255;
                cg += add; if (cg > 255) cg = 255;
                cb += add; if (cb > 255) cb = 255;
            }
        }

        int r2_8    = (r8 * r8) >> 8;              /* Q8 field-px^2           */
        int reach8  = r8 * 5 / 2;                  /* kernel negligible past this */
        int reach2_8 = (int)(((int64_t)reach8 * reach8) >> 8);
        /* subtract the kernel's value AT the cutoff so it reaches zero
         * smoothly — without this the bounding box shows as a hard
         * rectangular seam wherever a blob's influence stops */
        int edge = (r2_8 << 8) / reach2_8;
        int reach = (reach8 >> 8) + 1;
        int x0 = (cx8 >> 8) - reach, x1 = (cx8 >> 8) + reach;
        int y0 = (cy8 >> 8) - reach, y1 = (cy8 >> 8) + reach;
        if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
        if (x1 >= fw) x1 = fw - 1; if (y1 >= fh) y1 = fh - 1;

        for (int y = y0; y <= y1; y++) {
            int dy8 = (y << 8) - cy8, dy2 = (dy8 * dy8) >> 8;
            size_t row = (size_t)y * fw;
            uint16_t *frow = p201_field + row;
            uint32_t *rrow = p201_rs + row, *grow = p201_gs + row, *brow = p201_bs + row;
            for (int x = x0; x <= x1; x++) {
                int dx8 = (x << 8) - cx8;
                int d2 = (((dx8 * dx8) >> 8) + dy2);      /* Q8 field-px^2 */
                if (d2 < 16) d2 = 16;
                /* classic metaball kernel r^2/d^2, Q8, clamped */
                int v = (r2_8 << 8) / d2 - edge;
                if (v <= 0) continue;
                if (v > 4095) v = 4095;
                /* 7 blobs * 4095 = 28665: no clamp needed, and leaving the
                 * field unclamped keeps it consistent with the colour sums
                 * (a clamped field divided into unclamped sums oversaturates
                 * exactly where the wax is thickest). */
                frow[x] = (uint16_t)(frow[x] + v);
                rrow[x] += (uint32_t)v * (uint32_t)cr;
                grow[x] += (uint32_t)v * (uint32_t)cg;
                brow[x] += (uint32_t)v * (uint32_t)cb;
            }
        }
    }

    /* ---- normalise colour at field resolution --------------------------
     * One divide per quarter-res cell instead of three per output pixel. */
    for (size_t i = 0; i < ncell; i++) {
        uint32_t f = p201_field[i];
        if (!f) { p201_col[i] = 0; continue; }
        p201_col[i] = ((p201_rs[i] / f) << 16) | ((p201_gs[i] / f) << 8) | (p201_bs[i] / f);
    }

    /* ---- upsample -------------------------------------------------------
     * Field and colour are both bilinear.  Colour used to be sampled
     * nearest-neighbour, which drew the quarter-res grid as visible 4-pixel
     * stair-steps down every seam.  Work is hoisted per cell: the two
     * vertical mixes happen once for each group of DIV output pixels, and
     * only the horizontal mix is per-pixel. */
    static const uint32_t wxtab[P201_DIV] = { 0, 256 / P201_DIV,
                                              2 * (256 / P201_DIV),
                                              3 * (256 / P201_DIV) };
    for (int y = 0; y < h; y++) {
        int fy = y / P201_DIV; if (fy > fh - 2) fy = fh - 2;
        uint32_t wy = (uint32_t)(((y % P201_DIV) * 256) / P201_DIV);
        const uint16_t *f0 = p201_field + (size_t)fy * fw, *f1 = f0 + fw;
        const uint32_t *c0 = p201_col   + (size_t)fy * fw, *c1 = c0 + fw;
        uint32_t *out = fb + (size_t)y * w;

        int x = 0;
        for (int fx = 0; fx < fw - 1 && x < w; fx++) {
            int vL = f0[fx]     + (int)((((int)f1[fx]     - (int)f0[fx])     * (int)wy) >> 8);
            int vR = f0[fx + 1] + (int)((((int)f1[fx + 1] - (int)f0[fx + 1]) * (int)wy) >> 8);
            uint32_t colL = p201_mix(c0[fx],     c1[fx],     wy);
            uint32_t colR = p201_mix(c0[fx + 1], c1[fx + 1], wy);

            for (int k = 0; k < P201_DIV && x < w; k++, x++) {
                uint32_t wx = wxtab[k];
                int v = vL + (((vR - vL) * (int)wx) >> 8);
                if (v <= P201_THR) { out[x] = 0xFF000000u; continue; }

                uint32_t col = p201_mix(colL, colR, wx);

                /* soft shoulder just above the surface gives the wax its
                 * rounded edge without an alpha channel */
                int lift = v - P201_THR;
                if (lift < P201_SHLD) {
                    uint32_t kk = (uint32_t)lift * 256u / P201_SHLD;
                    uint32_t rb = (((col & 0x00FF00FFu) * kk) >> 8) & 0x00FF00FFu;
                    uint32_t gg = (((col & 0x0000FF00u) * kk) >> 8) & 0x0000FF00u;
                    col = rb | gg;
                }
                out[x] = 0xFF000000u | col;
            }
        }
        for (; x < w; x++) out[x] = 0xFF000000u;      /* right edge remainder */
    }
}
