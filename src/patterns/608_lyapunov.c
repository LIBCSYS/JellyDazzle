/* 608 Lyapunov — a fractal made out of stability, not escape.
 *
 * Everything else in this family asks "does this point run away to infinity?"
 * This one asks a different question, and gets a completely different picture.
 *
 * Take the logistic map x -> r·x·(1-x), the standard model of a population
 * that breeds and then starves. Now alternate the growth rate between two
 * values A and B according to a repeating pattern — AB, AABAB, BBABA, whatever
 * — and measure the LYAPUNOV EXPONENT: whether two nearly identical starting
 * populations converge (stable, negative exponent) or diverge (chaotic,
 * positive). Plot that over the A-B plane and you get Markus and Hessler's
 * "Zircon Zity": rounded organic swellings that look like something biological
 * or geological, with a completely different vocabulary from anything else in
 * the library — no filaments, no rigging, no basins.
 *
 * The pattern string is the shape. "AB" gives the famous swallow forms;
 * "AABAB" gives the layered terraces; longer strings give stranger scaffolds.
 * Each instance picks one, so this routine is really a dozen different
 * fractals sharing one implementation.
 *
 * COST, and how it is paid. The honest version needs a logarithm per
 * iteration, which is hundreds of logs per pixel — far too slow. But the
 * exponent is a SUM of logs, and a sum of logs is the log of a product. So the
 * derivatives are multiplied together and the log is taken once every 16
 * steps, which is exact (not an approximation) and sixteen times cheaper.
 * Rendered once, then animated by palette rotation like the rest.
 *
 * Nothing is black: stable and chaotic regions each get their own half of the
 * ramp, and the exponent shades continuously within.
 */
#include "../engine/jellydazzle.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

#define L608_WARM 48              /* iterations discarded before measuring   */
#define L608_MEAS 132             /* iterations actually accumulated         */
#define L608_CHUNK 16             /* how often the running product is logged */

/* The sequences. Each is a different fractal. */
static const char *const L608_SEQ[] = {
    "AB", "AABAB", "BBABA", "AAB", "ABB", "AABB",
    "ABAABBA", "BBBAAAB", "AABBAB", "ABABB", "AAABB", "BABA",
};
#define L608_NSEQ ((int)(sizeof L608_SEQ / sizeof L608_SEQ[0]))

/* Windows on the A-B plane worth looking at. Outside roughly [2,4] the map is
 * not interesting — populations just die or blow up. */
static const struct { double a0, b0, span; } L608_WIN[] = {
    { 2.00, 2.00, 2.00 },      /* the whole plane, the classic view      */
    { 3.20, 3.20, 0.80 },      /* the dense chaotic quarter              */
    { 2.40, 3.40, 0.90 },      /* the big swallow                        */
    { 3.40, 2.60, 0.60 },      /* terraces                               */
    { 2.85, 3.05, 0.45 },      /* the neck, where stable meets chaotic   */
    { 3.60, 3.60, 0.35 },      /* deep chaos, fine structure             */
};
#define L608_NWIN ((int)(sizeof L608_WIN / sizeof L608_WIN[0]))

static uint8_t *pl608;
static int      pw608, ph608;
static uint32_t seed608 = 0xFFFFFFFFu;
static int      row608;
static double   par608[8];
static const char *seq608;
static int      seqn608;

static uint32_t r608(uint32_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
    return *s;
}
static float f608(uint32_t *s) { return (float)(r608(s) >> 8) * (1.0f / 16777216.0f); }

static void plan608(int w, int h, uint32_t seed)
{
    size_t need = (size_t)w * h;
    if (pw608 != w || ph608 != h) {
        free(pl608); pl608 = (uint8_t *)malloc(need);
        pw608 = w; ph608 = h;
    }
    if (!pl608) return;
    memset(pl608, 0, need);
    row608 = 0;

    uint32_t s = seed ? seed : 0x7A9C0FFEu;
    int si = (int)(f608(&s) * L608_NSEQ); if (si >= L608_NSEQ) si = L608_NSEQ - 1;
    seq608 = L608_SEQ[si];
    seqn608 = (int)strlen(seq608);

    int wi = (int)(f608(&s) * L608_NWIN); if (wi >= L608_NWIN) wi = L608_NWIN - 1;
    double span = L608_WIN[wi].span * (0.70 + 0.55 * (double)f608(&s));
    /* Keep both rates inside [2,4]. Past 4 the logistic map just diverges and
     * the view grows a flat dead border, which is what the first render did. */
    double room = 4.0 - (L608_WIN[wi].a0 > L608_WIN[wi].b0 ? L608_WIN[wi].a0
                                                           : L608_WIN[wi].b0);
    if (span > room) span = room;

    par608[0] = L608_WIN[wi].a0;
    par608[1] = L608_WIN[wi].b0;
    par608[2] = span;
    par608[3] = 8.0 + (double)f608(&s) * 220.0;      /* palette base           */
    par608[4] = 26.0 + (double)f608(&s) * 46.0;      /* stable-side contrast   */
    par608[5] = 30.0 + (double)f608(&s) * 60.0;      /* chaotic-side contrast  */
    par608[6] = (double)f608(&s) < 0.5f ? 0.0 : 1.0; /* swap the axes          */
}

static void slice608(int w, int h, int rows)
{
    if (!pl608 || row608 >= h) return;
    double a0 = par608[0], b0 = par608[1], span = par608[2];
    int    base = (int)par608[3];
    double kst = par608[4], kch = par608[5];
    int    swap = par608[6] > 0.5;

    int yend = row608 + rows; if (yend > h) yend = h;
    for (int y = row608; y < yend; y++) {
        double fy = (double)y / (double)h;
        uint8_t *dst = pl608 + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            double fx = (double)x / (double)w;
            double a = a0 + (swap ? fy : fx) * span;
            double b = b0 + (swap ? fx : fy) * span;

            double xv = 0.5;
            int si = 0;
            for (int i = 0; i < L608_WARM; i++) {
                double r = (seq608[si] == 'A') ? a : b;
                xv = r * xv * (1.0 - xv);
                if (++si == seqn608) si = 0;
            }

            /* The exponent is the mean of log|r(1-2x)|. Multiply the terms and
             * take the log once per chunk — identical arithmetic, one
             * sixteenth of the transcendental calls. */
            double sum = 0.0, prod = 1.0;
            int    chunk = 0, used = 0;
            for (int i = 0; i < L608_MEAS; i++) {
                double r = (seq608[si] == 'A') ? a : b;
                xv = r * xv * (1.0 - xv);
                double d = fabs(r * (1.0 - 2.0 * xv));
                if (d < 1e-300) d = 1e-300;
                prod *= d;
                used++;
                if (++chunk == L608_CHUNK) { sum += log(prod); prod = 1.0; chunk = 0; }
                if (++si == seqn608) si = 0;
            }
            if (chunk) sum += log(prod);
            double lam = sum / (double)used;

            /* Stable below zero, chaotic above. Each gets its own half of the
             * ramp so the two regimes read as different materials, and both
             * are shaded — neither is a flat block and neither is black. */
            int idx;
            if (lam < 0.0) {
                double m = -lam; if (m > 3.0) m = 3.0;
                idx = base + 128 - (int)(sqrt(m / 3.0) * kst);
            } else {
                double m = lam; if (m > 1.2) m = 1.2;
                idx = base + 128 + (int)(sqrt(m / 1.2) * kch);
            }
            dst[x] = (uint8_t)(idx & 255);
        }
    }
    row608 = yend;
}

void pattern_608(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (seed != seed608 || pw608 != w || ph608 != h) {
        plan608(w, h, seed);
        seed608 = seed;
    }
    if (!pl608) return;
    if (row608 < h) slice608(w, h, h / 90 + 1);   /* ~90 frames to resolve */

    uint32_t rot = (uint32_t)frame * 13u;
    uint32_t lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = pal[((uint32_t)i * 128u + rot) & JD_PAL_MASK];
    const uint8_t *src = pl608;
    size_t n = (size_t)w * h;
    for (size_t i = 0; i < n; i++) fb[i] = lut[src[i]];
}
