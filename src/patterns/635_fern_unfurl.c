/* 635 Fern Unfurl (fractal) — Barnsley's fern, drawn the way it grows.
 *
 * Four affine maps, chosen with fixed probabilities, applied over and over to
 * a single moving point.  No geometry, no recursion, no model of a leaf: the
 * fern is the attractor of the maps and it appears because the point cannot
 * settle anywhere else.  One of the four maps is a near-collapse to the stem
 * and gets 1% of the draws; one is the 85% self-similar shrink-and-lift that
 * builds the whole frond out of smaller copies of itself; the other two are
 * the left and right pinnae.
 *
 * The unfurling is a gate, not a different algorithm.  The chaos game has to
 * run continuously — stop stepping it and the point leaves the attractor, and
 * restarting costs a warm-up — so every iteration is computed, and a rising
 * height threshold decides which ones are allowed to draw.  The threshold
 * climbs from the base to above the tip, holds there, then the canvas is
 * cleared and it starts again.  Points landing just under the threshold are
 * drawn hotter, which puts a bright growing edge at the front.
 *
 * COST STRATEGY.  This is the naturally cheap half of the family: a FIXED
 * BUDGET of chaos-game steps per frame (about 2,600), each one four multiplies
 * and an add, plotted as a very small dot into a slowly decaying canvas.  No
 * grid, no escape loop, no per-pixel pass.  The fern stays bright not because
 * anything is retained but because the attractor is re-hit thousands of times
 * a second in the same places, so a decay around 0.99 holds the picture while
 * still letting the ungrown region above the threshold stay dark.
 */
#include "_spark572.h"

#define FN635_PTS   2600          /* chaos-game steps per frame           */
#define FN635_GROW  900           /* frames for the front to reach the tip */
#define FN635_HOLD  520           /* frames fully grown before reset       */

static gk       g635;
static uint32_t bs635 = 0xFFFFFFFFu;
static int      base635;
static float    fx635, fy635;     /* the wandering point                  */
static int      age635;           /* frames since this fern started        */
static float    hue635;
static float    lean635;          /* a small shear, so it is not identical */

void pattern_635(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g635, w, h);
    gk_decay_snap(&g635, 0.990f);

    float cw = (float)g635.cw, ch = (float)g635.ch, sc = g635.sc;

    if (seed != bs635) {
        base635 = (int)(seed & 0x7FFFu);
        fx635   = 0.0f;
        fy635   = 0.0f;
        age635  = 0;
        hue635  = gk_hash(seed + 61u);
        lean635 = (gk_hash(seed + 62u) - 0.5f) * 0.12f;
        gk_seed(&g635, seed ^ 0xBEEF635u);
        gk_clear(&g635);
        bs635 = seed;
    }

    /* The fern lives in x in about [-2.2, 2.7], y in [0, 10].  Map that to the
     * canvas with a single uniform scale so it is not stretched, bottom of the
     * stem a little above the canvas floor. */
    float S  = ch * 0.092f;
    float ox = cw * 0.5f;
    float oy = ch * 0.965f;

    /* The growth front, in fern units. */
    float front;
    if (age635 < FN635_GROW) {
        float u = (float)age635 / (float)FN635_GROW;
        /* Ease out: the tip takes longer than the base, which is how a frond
         * actually opens.  A linear front looks like a wipe. */
        front = 10.6f * (1.0f - (1.0f - u) * (1.0f - u));
    } else {
        front = 10.6f;
    }
    if (age635 > FN635_GROW + FN635_HOLD) {      /* start over */
        gk_clear(&g635);
        age635 = 0;
        hue635 = gk_hash((uint32_t)frame ^ 0x9E37u);
        front  = 0.0f;
    }
    age635++;

    float t = (float)frame;
    float col[3];
    float core = 0.55f * sc, glow = 1.45f * sc;

    for (i = 0; i < FN635_PTS; i++) {
        float p = gk_rf(&g635);
        float nx, ny;
        if (p < 0.01f) {                          /* 1% — collapse to stem  */
            nx = 0.0f;
            ny = 0.16f * fy635;
        } else if (p < 0.86f) {                   /* 85% — the self-copy    */
            nx =  0.85f * fx635 + 0.04f * fy635;
            ny = -0.04f * fx635 + 0.85f * fy635 + 1.60f;
        } else if (p < 0.93f) {                   /* 7% — right pinna       */
            nx =  0.20f * fx635 - 0.26f * fy635;
            ny =  0.23f * fx635 + 0.22f * fy635 + 1.60f;
        } else {                                  /* 7% — left pinna        */
            nx = -0.15f * fx635 + 0.28f * fy635;
            ny =  0.26f * fx635 + 0.24f * fy635 + 0.44f;
        }
        fx635 = nx; fy635 = ny;

        if (fy635 > front) continue;              /* not grown yet          */

        /* A little shear that grows with height, plus a very slow sway, so the
         * frond is not a static diagram.  Applied at draw time only — feeding
         * it back into the maps would move the attractor. */
        float sway = lean635 + 0.016f * sinf(t * 0.0045f);
        float px = ox + (fx635 + sway * fy635) * S;
        float py = oy - fy635 * S;
        if (px < -4.0f || px > cw + 4.0f || py < -4.0f) continue;

        /* Hue up the frond, and a hot band just under the front so the growing
         * edge reads as new tissue rather than as a clipping plane. */
        float near = front - fy635;
        float edge = near < 0.55f ? (1.0f - near * 1.81f) : 0.0f;
        float hue  = hue635 + fy635 * 0.030f + edge * 0.17f;
        float amp  = 0.55f + 0.55f * edge;
        sk_col(pal, sk_hidx(base635, hue + t * 0.00016f),
               0.14f + 0.40f * edge, 0.46f, amp, col);
        gk_dot(&g635, px, py, col,
               core * (1.0f + 0.5f * edge), glow, 0.16f + 0.14f * edge);
    }

    gk_present(&g635, fb, w, h);
}
