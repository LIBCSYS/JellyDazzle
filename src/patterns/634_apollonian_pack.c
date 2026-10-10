/* 634 Apollonian Pack (fractal) — circles all the way down.
 *
 * Take three mutually tangent circles inside a fourth.  Every gap between
 * three of them admits exactly one more circle tangent to all three, and that
 * creates three new gaps, forever.  The Apollonian gasket is the limit, and it
 * is the only fractal in this library that is built out of nothing but
 * circles — which is why it looks structural rather than organic next to the
 * escape-time sets.
 *
 * It is also the cheapest to generate, because of Descartes' circle theorem.
 * Given a tangent quadruple with curvatures k1..k4, the second solution for
 * the fourth is k' = 2(k1+k2+k3) - k4, and the complex form gives the centre
 * by the same linear combination.  No square roots, no iteration, no search —
 * each new circle is six multiplies.  An earlier attempt solved the quadratic
 * form with a complex square root and spent its time choosing the right
 * branch; the linear recursion makes the branch choice for free by remembering
 * which member of the quadruple it is replacing.
 *
 * COST STRATEGY.  The gasket is built ONCE, at reseed: a recursive descent
 * that stops at a minimum radius or a fixed ceiling on the circle count, with
 * a duplicate check against the circles already recorded.  That is a one-off
 * cost of a few hundred circles and is not paid again.  Per frame the pattern
 * draws a FIXED BUDGET of rings taken from that list, advancing the start
 * index each frame so it cycles through the whole gasket every few frames; the
 * canvas decay holds the rings drawn on previous frames, so the complete
 * gasket is always on screen while only a slice of it is being rasterised.
 * The breathing is deliberately very slow — a cycle takes most of a minute —
 * so that rings drawn on consecutive frames land within a pixel of each other
 * and the cycling reads as a shimmer travelling over the packing, not a smear.
 */
#include "_spark572.h"

#define AP634_MAX   400           /* ceiling on recorded circles          */
#define AP634_RMIN  0.0042f       /* smallest circle worth drawing        */
#define AP634_DEPTH 9
#define AP634_BUDG  110           /* rings rasterised per frame           */

static gk       g634;
static uint32_t bs634 = 0xFFFFFFFFu;
static int      base634;
static int      cur634;           /* cycling start index into the list    */

/* curvature, centre x, centre y — index 0 is never the outer circle */
static float k634[AP634_MAX], cx634[AP634_MAX], cy634[AP634_MAX];
static int   n634;

/* Record a circle unless an equal one is already present.  The recursion
 * reaches the same circle by several routes, so without this the list fills
 * with duplicates and the depth limit is wasted on them.  Tolerance scales
 * with the radius because large circles are reached often and small ones
 * carry more positional error. */
static int ap634_add(float k, float x, float y)
{
    int i;
    float r = 1.0f / k;
    float tol = r * 0.08f;
    for (i = 0; i < n634; i++) {
        if (fabsf(k634[i] - k) < k * 0.02f
         && fabsf(cx634[i] - x) < tol
         && fabsf(cy634[i] - y) < tol) return 0;
    }
    if (n634 >= AP634_MAX) return 0;
    k634[n634] = k; cx634[n634] = x; cy634[n634] = y;
    n634++;
    return 1;
}

static void ap634_rec(const float *A, const float *B, const float *C,
                      const float *D, int depth);

/* Replace S in the quadruple (P,Q,R,S) with the other circle tangent to
 * P, Q and R.  This is Descartes in its linear form. */
static void ap634_child(const float *P, const float *Q, const float *R,
                        const float *S, int depth)
{
    float k = 2.0f * (P[0] + Q[0] + R[0]) - S[0];
    if (k <= 0.0f) return;                       /* would be the outer again */
    float r = 1.0f / k;
    if (r < AP634_RMIN) return;

    float x = (2.0f * (P[0] * P[1] + Q[0] * Q[1] + R[0] * R[1])
               - S[0] * S[1]) * r;
    float y = (2.0f * (P[0] * P[2] + Q[0] * Q[2] + R[0] * R[2])
               - S[0] * S[2]) * r;

    /* Must sit inside the unit outer circle.  Floating point drift at depth
     * occasionally produces a circle just outside it; dropping those is
     * cheaper than carrying doubles through the whole recursion. */
    float lim = 1.0f - r;
    if (x * x + y * y > lim * lim + 0.002f) return;

    if (!ap634_add(k, x, y)) return;
    if (depth <= 0 || n634 >= AP634_MAX) return;

    float N[3]; N[0] = k; N[1] = x; N[2] = y;
    ap634_rec(P, Q, R, N, depth - 1);
}

/* Keep D, replace each of A, B, C in turn.  Enumerating it this way reaches
 * each gap once instead of bouncing back to the parent. */
static void ap634_rec(const float *A, const float *B, const float *C,
                      const float *D, int depth)
{
    ap634_child(B, C, D, A, depth);
    ap634_child(A, C, D, B, depth);
    ap634_child(A, B, D, C, depth);
}

void pattern_634(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g634, w, h);
    gk_decay_snap(&g634, 0.93f);

    float cw = (float)g634.cw, ch = (float)g634.ch, sc = g634.sc;

    if (seed != bs634) {
        base634 = (int)(seed & 0x7FFFu);
        cur634  = 0;
        n634    = 0;

        /* The (-1, 2, 2, 3) gasket.  Outer circle of radius 1 at the origin,
         * two halves of it, and the first circle in the gap above them. */
        float C0[3] = { -1.0f,  0.0f,  0.0f     };
        float C1[3] = {  2.0f, -0.5f,  0.0f     };
        float C2[3] = {  2.0f,  0.5f,  0.0f     };
        float C3[3] = {  3.0f,  0.0f, -0.666667f };

        ap634_add(C1[0], C1[1], C1[2]);
        ap634_add(C2[0], C2[1], C2[2]);
        ap634_add(C3[0], C3[1], C3[2]);

        /* First the mirror of C3 through the real axis, which the keep-D
         * enumeration below cannot reach, then the rest of the descent. */
        ap634_child(C0, C1, C2, C3, AP634_DEPTH);
        ap634_rec(C0, C1, C2, C3, AP634_DEPTH);

        gk_clear(&g634);
        bs634 = seed;
    }

    float t = (float)frame;

    /* Breathing: one slow cycle, plus a drift in the overall rotation so the
     * packing is not always presented at the same angle. */
    float breathe = 1.0f + 0.045f * sinf(t * 0.0065f);
    float rot     = t * 0.00055f;
    float cs = cosf(rot), sn = sinf(rot);
    float R  = (cw < ch ? cw : ch) * 0.455f * breathe;
    float ox = cw * 0.5f, oy = ch * 0.5f;
    float wd = 1.15f * sc;
    float col[3];

    /* The outer boundary every frame — it anchors the composition, and it is
     * one ring, so it is not worth cycling. */
    sk_col(pal, sk_hidx(base634, 0.02f + t * 0.00018f), 0.30f, 0.45f, 0.55f, col);
    gk_ring(&g634, ox, oy, R, wd * 1.6f, col);

    if (n634 > 0) {
        int budget = AP634_BUDG < n634 ? AP634_BUDG : n634;
        for (i = 0; i < budget; i++) {
            int idx = (cur634 + i) % n634;
            float r = (1.0f / k634[idx]) * R;
            if (r < 0.8f) continue;                  /* below a pixel: skip */

            float lx = cx634[idx] * R, ly = cy634[idx] * R;
            float x = ox + lx * cs - ly * sn;
            float y = oy + lx * sn + ly * cs;

            /* Hue from curvature.  Circle size falls off geometrically with
             * depth in the packing, so the log of the curvature is close to
             * the generation number — the hue ramp reads as depth. */
            float hue = 0.10f + logf(k634[idx]) * 0.055f + t * 0.00018f;
            float amp = 0.34f + 0.42f / (1.0f + logf(k634[idx]) * 0.45f);
            sk_col(pal, sk_hidx(base634, hue), 0.22f, 0.48f, amp, col);
            gk_ring(&g634, x, y, r, wd, col);
        }
        cur634 = (cur634 + budget) % n634;
    }

    gk_present(&g634, fb, w, h);
}
