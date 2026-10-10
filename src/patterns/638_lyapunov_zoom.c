/* 638 Lyapunov Zoom (fractal) — a stability map of the logistic map.
 *
 * Nothing else here is a picture of a measurement.  For each point (a, b) the
 * logistic map x -> r x (1-x) is run with r taken alternately from a and b
 * following a fixed sequence, and the Lyapunov exponent of that run is
 * computed: the average of log |r (1 - 2x)| over the orbit.  Negative means
 * nearby starting points converge, so the orbit is stable; positive means they
 * separate, so it is chaotic.  Colour the plane by that number and what
 * appears is the structure Mario Markus found in the eighties — the stable
 * regions form rounded interlocking shapes with a hard chaotic sea between
 * them, the whole thing looking far more like wet enamel or a cityscape than
 * like any escape-time set.
 *
 * The sequence drives the shape.  "AB" gives the classic view.  Longer words
 * such as BBABA fold the stable regions into each other and are worth
 * visiting, so the pattern picks one at reseed and keeps it; the motion is a
 * slow zoom and drift across (a, b) space toward a hand-picked target and back
 * out again, rather than changing the sequence underneath the viewer.
 *
 * COST STRATEGY.  The exponent is the expensive quantity in this library — it
 * wants dozens of logistic steps per sample where an escape-time set often
 * bails in eight.  Three things pay for it.  First, a coarser grid than the
 * escape-time patterns use: cells about six and a half canvas pixels.  Second,
 * a rolling sweep of eight grid rows per frame with the canvas decay holding
 * the rest, so under nine hundred samples are taken per frame.  Third, and the
 * real saving: the logarithm is NOT taken per iteration.  The derivative
 * magnitudes are multiplied into a running product and the log is taken once
 * every eight steps, which cuts the transcendental count by eight for an
 * identical result.  The product is floored before the log because x landing
 * near one half drives a factor to zero and would otherwise produce a negative
 * infinity.
 */
#include "_spark572.h"

#define LY638_CELL  6.5f
#define LY638_BAND  8
#define LY638_WARM  16            /* iterations discarded                 */
#define LY638_MEAS  56            /* iterations averaged                  */
#define LY638_BATCH 8             /* steps per logf                       */

static gk       g638;
static uint32_t bs638 = 0xFFFFFFFFu;
static int      base638;
static int      row638;
static float    zoom638;          /* 1 = wide, small = deep               */
static int      dir638;           /* +1 diving, -1 surfacing              */
static int      seq638[8];        /* 0 = a, 1 = b                         */
static int      slen638;
static float    ta638, tb638;     /* dive target in (a,b)                 */
static float    hue638;

/* Sequences worth looking at, as a/b patterns.  "AB" is the classic; the
 * longer words bend the stable regions in ways that are not obvious from it. */
static const char *SEQ638[5] = { "AB", "BBABA", "AABB", "ABBB", "BBBBBBAAAAAA" };

/* Targets inside the interesting band.  Outside roughly 2.0..4.0 on both axes
 * the map is either trivially stable or divergent, and a dive at a random
 * point lands in flat colour within two doublings. */
static const float TG638[4][2] = {
    { 3.82f, 3.42f },
    { 3.40f, 3.08f },
    { 2.72f, 3.86f },
    { 3.55f, 3.62f }
};

void pattern_638(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int gx, gy, i;
    gk_setup(&g638, w, h);
    gk_decay_snap(&g638, 0.90f);

    float cw = (float)g638.cw, ch = (float)g638.ch, sc = g638.sc;

    if (seed != bs638) {
        base638 = (int)(seed & 0x7FFFu);
        int which = (int)(gk_hash(seed + 91u) * 5.0f) % 5;
        const char *s = SEQ638[which];
        slen638 = 0;
        for (i = 0; s[i] && slen638 < 8; i++)
            seq638[slen638++] = (s[i] == 'B') ? 1 : 0;
        if (slen638 < 2) { seq638[0] = 0; seq638[1] = 1; slen638 = 2; }

        int tg = (int)(gk_hash(seed + 92u) * 4.0f) % 4;
        ta638   = TG638[tg][0];
        tb638   = TG638[tg][1];
        hue638  = gk_hash(seed + 93u);
        zoom638 = 1.0f;
        dir638  = 1;
        row638  = 0;
        gk_clear(&g638);
        bs638 = seed;
    }

    float cell  = LY638_CELL * sc;
    int   gcols = (int)(cw / cell) + 1;
    int   grows = (int)(ch / cell) + 1;

    /* The wide view is the whole interesting square; the dive narrows it
     * around the target. */
    float wide = 1.00f;
    float span = wide * zoom638;
    float a0   = ta638 - span, a1 = ta638 + span;
    float b0   = tb638 - span, b1 = tb638 + span;
    /* Clamp the wide view back into the band — off the edges the logistic map
     * either collapses or diverges and the colour goes flat. */
    if (a0 < 2.00f) { a1 += 2.00f - a0; a0 = 2.00f; }
    if (a1 > 4.00f) { a0 -= a1 - 4.00f; a1 = 4.00f; if (a0 < 2.0f) a0 = 2.0f; }
    if (b0 < 2.00f) { b1 += 2.00f - b0; b0 = 2.00f; }
    if (b1 > 4.00f) { b0 -= b1 - 4.00f; b1 = 4.00f; if (b0 < 2.0f) b0 = 2.0f; }

    float t    = (float)frame;
    float da   = (a1 - a0), db = (b1 - b0);
    float inv  = 1.0f / (float)LY638_MEAS;
    float col[3];

    int ylast = row638 + LY638_BAND;
    if (ylast > grows) ylast = grows;

    for (gy = row638; gy < ylast; gy++) {
        float py = ((float)gy + 0.5f) * cell;
        float bb = b0 + (py / ch) * db;
        for (gx = 0; gx < gcols; gx++) {
            float px = ((float)gx + 0.5f) * cell;
            float aa = a0 + (px / cw) * da;

            float rr[2]; rr[0] = aa; rr[1] = bb;
            float x = 0.5f;
            int   si = 0;

            /* warm-up: let the orbit settle before measuring anything */
            for (i = 0; i < LY638_WARM; i++) {
                x = rr[seq638[si]] * x * (1.0f - x);
                if (++si >= slen638) si = 0;
            }

            /* Measurement.  The product-then-log batching is the whole reason
             * this pattern is affordable — see the header note. */
            float sum = 0.0f, prod = 1.0f;
            int   nb  = 0;
            for (i = 0; i < LY638_MEAS; i++) {
                float r = rr[seq638[si]];
                if (++si >= slen638) si = 0;
                prod *= fabsf(r * (1.0f - 2.0f * x));
                x = r * x * (1.0f - x);
                if (++nb == LY638_BATCH) {
                    if (prod < 1e-20f) prod = 1e-20f;   /* x hit the middle */
                    sum += logf(prod);
                    prod = 1.0f; nb = 0;
                }
            }
            if (nb) {
                if (prod < 1e-20f) prod = 1e-20f;
                sum += logf(prod);
            }
            float lam = sum * inv;                      /* the exponent     */

            /* Two colour regimes, because the quantity means two different
             * things either side of zero.  Stable regions get a deep narrow
             * band keyed to how stable they are; the chaotic sea gets a
             * separate, brighter band.  Mapping both through one ramp, which
             * the first version did, loses the boundary entirely — and the
             * boundary is the fractal. */
            float hue, amp;
            if (lam < 0.0f) {
                float d = -lam;
                if (d > 2.4f) d = 2.4f;
                hue = hue638 + 0.04f + d * 0.115f;
                amp = 0.24f + 0.62f * (d * (1.0f / 2.4f));
            } else {
                float d = lam;
                if (d > 1.1f) d = 1.1f;
                hue = hue638 + 0.56f + d * 0.16f;
                amp = 0.80f - 0.42f * (d * (1.0f / 1.1f));
            }
            sk_col(pal, sk_hidx(base638, hue + t * 0.00018f),
                   0.18f, 0.52f, amp, col);
            gk_dot(&g638, px, py, col, cell * 0.46f, cell * 0.64f, 0.26f);
        }
    }

    row638 = ylast;
    if (row638 >= grows) {              /* sweep done: move the view        */
        row638 = 0;
        if (dir638 > 0) {
            zoom638 *= 0.966f;
            if (zoom638 < 0.012f) dir638 = -1;
        } else {
            zoom638 *= 1.045f;          /* back out faster than it went in  */
            if (zoom638 >= 1.0f) {
                zoom638 = 1.0f;
                dir638  = 1;
                int tg = (int)(gk_hash((uint32_t)frame ^ 0x638u) * 4.0f) % 4;
                ta638  = TG638[tg][0];
                tb638  = TG638[tg][1];
            }
        }
    }

    gk_present(&g638, fb, w, h);
}
