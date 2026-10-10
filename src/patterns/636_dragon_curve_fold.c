/* 636 Dragon Curve Fold (fractal) — a strip of paper folding itself up.
 *
 * The Heighway dragon is what you get if you fold a strip of paper in half
 * repeatedly, then open every crease to exactly ninety degrees.  The fold
 * directions are not random and do not need storing: for segment i the crease
 * turns right whenever ((i & -i) << 1) & i is non-zero, which is a property of
 * the binary carry pattern.  Two integer operations per segment and the whole
 * curve is determined.
 *
 * The animation is the opening of the creases.  At zero degrees the curve is a
 * straight line — the unfolded strip.  As the crease angle opens toward ninety
 * the line gathers itself into the dragon, and it goes back the other way.
 * Watching it reveals something a still picture hides: the dragon's apparent
 * symmetry is the fold structure, and you can see which parts are copies of
 * which as they swing into place together.
 *
 * COST STRATEGY.  Order nine, so 512 segments — enough that the folded form
 * reads as a dragon rather than a zigzag, few enough that the whole polyline
 * is rebuilt from scratch every frame for 512 sine/cosine pairs and 512 short
 * segment rasterisations.  That is cheaper than any grid sweep in this family
 * and needs no persistent state at all beyond the fold phase.  The canvas is
 * cleared rather than decayed each frame — a memset is cheaper than the decay
 * multiply, and a crisp curve suits this one better than a trail.  Because the
 * curve's extent changes enormously between unfolded and folded, the bounding
 * box is measured each frame and the fit recomputed, which keeps it filling
 * the canvas at every stage instead of shrinking to a dot.
 */
#include "_spark572.h"

#define DR636_ORD  9
#define DR636_N    (1 << DR636_ORD)        /* 512 segments                */

static gk       g636;
static uint32_t bs636 = 0xFFFFFFFFu;
static int      base636;
static float    ph636;            /* fold phase                          */
static float    rate636;
static float    spin636;
static float    hue636;

static float px636[DR636_N + 1], py636[DR636_N + 1];

void pattern_636(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g636, w, h);
    gk_clear(&g636);

    float cw = (float)g636.cw, ch = (float)g636.ch, sc = g636.sc;

    if (seed != bs636) {
        base636 = (int)(seed & 0x7FFFu);
        ph636   = gk_hash(seed + 71u) * GK_TAU;
        rate636 = 0.0030f + 0.0026f * gk_hash(seed + 72u);
        spin636 = (gk_hash(seed + 73u) - 0.5f) * 0.0012f;
        hue636  = gk_hash(seed + 74u);
        bs636 = seed;
    }

    float t = (float)frame;
    ph636 += rate636;
    if (ph636 > GK_TAU) ph636 -= GK_TAU;

    /* Crease angle, and back again.  The cosine ease lingers at both ends,
     * which is wanted at the folded end and was a mistake at the other: a
     * fully unfolded strip is a one-pixel line across the canvas, and with the
     * ease it sat there, nearly invisible, for half of every cycle.  The fold
     * therefore bottoms out at 0.30 — about twenty-seven degrees — which is
     * still clearly a strip coming apart but has enough extent to fill the
     * frame and carry its colour gradient. */
    float fold  = 0.30f + 0.70f * (0.5f - 0.5f * cosf(ph636));
    float theta = 1.5707963f * fold;

    /* Walk the creases.  Direction turns by +/- theta at each one; the turn
     * direction comes straight out of the segment index. */
    float ang = t * spin636;
    float x = 0.0f, y = 0.0f;
    px636[0] = 0.0f; py636[0] = 0.0f;
    float mnx = 0.0f, mxx = 0.0f, mny = 0.0f, mxy = 0.0f;
    for (i = 1; i <= DR636_N; i++) {
        x += cosf(ang);
        y += sinf(ang);
        px636[i] = x; py636[i] = y;
        if (x < mnx) mnx = x; else if (x > mxx) mxx = x;
        if (y < mny) mny = y; else if (y > mxy) mxy = y;
        /* the crease at the next joint */
        ang += (((i & -i) << 1) & i) ? -theta : theta;
    }

    /* Fit.  A single uniform scale taken from whichever axis is tighter, so
     * the unfolded strip stays a straight line instead of being stretched
     * vertically into noise by a degenerate bounding box. */
    float spx = mxx - mnx, spy = mxy - mny;
    if (spx < 1e-4f) spx = 1e-4f;
    if (spy < 1e-4f) spy = 1e-4f;
    float s1 = cw * 0.86f / spx, s2 = ch * 0.86f / spy;
    float S  = s1 < s2 ? s1 : s2;
    float ox = cw * 0.5f - (mnx + mxx) * 0.5f * S;
    float oy = ch * 0.5f - (mny + mxy) * 0.5f * S;

    /* Line weight tracks segment length.  At full fold each segment is tens of
     * pixels and wants a visible core; unfolded they are a fraction of a pixel
     * each and a thick core would just be a bright bar. */
    float seglen = S;
    float core = 0.45f * sc + 0.030f * seglen;
    float glow = 1.30f * sc + 0.075f * seglen;
    if (core > 1.9f * sc) core = 1.9f * sc;
    if (glow > 4.5f * sc) glow = 4.5f * sc;

    float col[3];
    float ax = ox + px636[0] * S, ay = oy + py636[0] * S;
    for (i = 1; i <= DR636_N; i++) {
        float bx = ox + px636[i] * S, by = oy + py636[i] * S;
        /* Hue by position ALONG the curve.  This is the point of the whole
         * pattern: the gradient labels the strip, so when it folds you can see
         * which stretches of paper end up next to each other. */
        float u   = (float)i * (1.0f / (float)DR636_N);
        float hue = hue636 + u * 0.72f + t * 0.00022f;
        sk_col(pal, sk_hidx(base636, hue), 0.26f, 0.44f,
               0.72f + 0.26f * fold, col);
        gk_seg(&g636, ax, ay, bx, by, col, core, glow, 0.20f);
        ax = bx; ay = by;
    }

    gk_present(&g636, fb, w, h);
}
