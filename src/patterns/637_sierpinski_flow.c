/* 637 Sierpinski Flow (fractal) — the triangle arriving one point at a time.
 *
 * The chaos game is the cheapest fractal there is.  Pick three vertices, put a
 * point anywhere, and repeatedly jump it halfway toward a vertex chosen at
 * random.  The Sierpinski triangle falls out, and it falls out whatever you
 * start with, because every point of the triangle is the limit of some
 * sequence of halvings and nothing else is.  It is one average and one
 * multiply per step.
 *
 * Drawing the points as they are generated is the obvious thing and it looks
 * like static.  Instead each chaos-game point becomes a TARGET, and a mote
 * falls into it from above the canvas — so the triangle does not appear, it
 * accumulates, with rain coming down into it continuously.  A mote brightens
 * as it closes on its target and stamps hard on landing, then takes a new
 * target from the walk and starts again from the top.  The landings are what
 * build the picture; the falling motes are what makes it alive.
 *
 * COST STRATEGY.  Fixed budget, no grid, no escape loop: 760 motes, one small
 * dot each per frame, plus roughly a tenth of them landing on any given frame
 * and stamping a second brighter dot.  The chaos walk itself runs a couple of
 * steps per landing, which is a few hundred steps a frame — negligible.  The
 * canvas decays slowly rather than being cleared, so landed points persist and
 * the triangle holds; it survives at that decay rate because the walk keeps
 * re-hitting the same attractor, not because anything is stored.  Total work
 * per frame is well under a thousand small dots.
 */
#include "_spark572.h"

#define SF637_N    760            /* motes in flight                      */

static gk       g637;
static uint32_t bs637 = 0xFFFFFFFFu;
static int      base637;
static float    wx637, wy637;     /* the chaos walk, in unit coordinates  */
static float    tx637[SF637_N], ty637[SF637_N];   /* landing targets      */
static float    sx637[SF637_N];                    /* launch x            */
static float    pr637[SF637_N];                    /* fall progress 0..1  */
static float    rt637[SF637_N];                    /* fall rate           */
static float    hu637[SF637_N];
static float    hue637;
static float    vx637[3], vy637[3];                /* triangle vertices   */

/* Advance the chaos walk one jump and read off the point. */
static void sf637_step(void)
{
    int k = (int)(gk_rf(&g637) * 3.0f);
    if (k > 2) k = 2;
    wx637 = 0.5f * (wx637 + vx637[k]);
    wy637 = 0.5f * (wy637 + vy637[k]);
}

static void sf637_arm(int i, float cw)
{
    sf637_step();
    sf637_step();                      /* two jumps: decorrelate neighbours */
    tx637[i] = wx637;
    ty637[i] = wy637;
    /* Launch from above, offset a little sideways so the fall has a slant and
     * the motes do not all travel on perfectly vertical lines. */
    sx637[i] = wx637 + (gk_rf(&g637) - 0.5f) * 0.22f * cw;
    pr637[i] = 0.0f;
    rt637[i] = 0.016f + 0.030f * gk_rf(&g637);
    hu637[i] = gk_rf(&g637);
}

void pattern_637(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g637, w, h);
    gk_decay_snap(&g637, 0.9895f);

    float cw = (float)g637.cw, ch = (float)g637.ch, sc = g637.sc;

    if (seed != bs637) {
        base637 = (int)(seed & 0x7FFFu);
        hue637  = gk_hash(seed + 81u);
        gk_seed(&g637, seed ^ 0x51E6377u);

        /* An equilateral triangle, upright, inset from the edges.  A slight
         * per-seed rotation so it is not the same diagram every time. */
        float rr = (cw < ch ? cw : ch) * 0.435f;
        float a0 = -1.5707963f + (gk_hash(seed + 82u) - 0.5f) * 0.5f;
        for (i = 0; i < 3; i++) {
            float a = a0 + (float)i * 2.0943951f;
            vx637[i] = cw * 0.5f + rr * cosf(a);
            vy637[i] = ch * 0.52f + rr * sinf(a);
        }
        wx637 = cw * 0.5f; wy637 = ch * 0.5f;
        for (i = 0; i < 40; i++) sf637_step();    /* settle onto the set   */
        for (i = 0; i < SF637_N; i++) {
            sf637_arm(i, cw);
            pr637[i] = gk_rf(&g637);              /* stagger the first wave */
        }
        gk_clear(&g637);
        bs637 = seed;
    }

    float t = (float)frame;
    float col[3];
    float top = -0.18f * ch;

    for (i = 0; i < SF637_N; i++) {
        pr637[i] += rt637[i];
        if (pr637[i] >= 1.0f) {
            /* LANDING.  This stamp is the one that stays — the triangle is
             * built entirely out of landings, so it gets the bright core and
             * the whole of the glow. */
            sk_col(pal, sk_hidx(base637, hue637 + hu637[i] * 0.22f + t * 0.00016f),
                   0.34f, 0.40f, 1.00f, col);
            gk_dot(&g637, tx637[i], ty637[i], col, 0.90f * sc, 2.30f * sc, 0.34f);
            sf637_arm(i, cw);
            continue;
        }

        /* Ease in: the mote accelerates downward, so the rain reads as falling
         * under gravity rather than sliding at a constant rate. */
        float u = pr637[i];
        float e = u * u;
        float x = sx637[i] + (tx637[i] - sx637[i]) * e;
        float y = top      + (ty637[i] - top)      * e;

        /* Brighten on approach.  Far out it is a faint streak of drizzle; the
         * last fifth of the fall is where it becomes a visible mote. */
        float near = u * u * u;
        float amp  = 0.10f + 0.62f * near;
        sk_col(pal, sk_hidx(base637, hue637 + hu637[i] * 0.22f + 0.10f + t * 0.00016f),
               0.18f + 0.22f * near, 0.48f, amp, col);
        gk_dot(&g637, x, y, col,
               0.45f * sc + 0.35f * sc * near, 1.50f * sc, 0.17f);
    }

    gk_present(&g637, fb, w, h);
}
