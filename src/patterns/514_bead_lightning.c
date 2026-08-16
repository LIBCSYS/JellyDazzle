/* 514 Bead Lightning — the rare decay of a channel into a string of
 * beads: a bolt grows down over ~25 frames and burns hot, then the
 * continuous channel dims while, along its length, glowing beads swell
 * out of it (every few segments, sizes and hues varying bead to bead) and
 * linger, cooling toward a deeper palette hue for a hundred-odd frames
 * before dissolving.  Two bolt slots on staggered clocks.  Sparse
 * overlay.  Repaint. */
#include "_trace509.h"

#define P514 330

static gk g514;
static gk_bolt b514[2];
static int bi514[2] = { -1, -1 };
static uint32_t bs514 = 0xFFFFFFFFu;
static float hue514[2];

void pattern_514(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g514, w, h);
    gk_clear(&g514);
    if (seed != bs514) { bi514[0] = bi514[1] = -1; bs514 = seed; }
    float cw = (float)g514.cw, ch = (float)g514.ch, sc = g514.sc, t = (float)frame;
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    for (int s = 0; s < 2; s++) {
        int ph = frame + s * (P514 / 2);
        int idx = ph / P514;
        float age = (float)(ph - idx * P514);
        if (idx != bi514[s]) {
            gk_seed(&g514, seed ^ (uint32_t)(idx * 5303 + s * 6197));
            float x0 = cw * (0.2f + 0.6f * gk_rf(&g514));
            gk_bolt_gen(&g514, &b514[s], x0, -ch * 0.02f, x0 + cw * 0.22f * gk_rs(&g514),
                        ch * (0.8f + 0.18f * gk_rf(&g514)), 0.19f, 7, 4, 0.35f);
            hue514[s] = gk_rf(&g514);
            bi514[s] = idx;
        }
        int pi = base + (int)(hue514[s] * 8000.0f);
        /* continuous channel: grows 0..25, hot to ~70, gone by ~140 */
        float cenv = gk_env(age, 8.0f, 60.0f, 70.0f);
        if (cenv > 0.0f) {
            float c0[3], c1[3], h0[3], h1[3];
            gk_col(pal, pi, 0.05f, 0.45f * cenv, h0);
            gk_col(pal, pi + 1200, 0.05f, 0.40f * cenv, h1);
            gk_col(pal, pi + 200, 0.65f, 0.75f * cenv, c0);
            gk_col(pal, pi + 1500, 0.5f, 0.65f * cenv, c1);
            bx_draw_grad(&g514, &b514[s], 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, age / 25.0f, 1.0f, h0, h1, 0.15f, 2.0f * sc, 7.0f * sc, 0.5f);
            bx_draw_grad(&g514, &b514[s], 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, age / 25.0f, 1.0f, c0, c1, 0.15f, 0.9f * sc, 2.4f * sc, 0.25f);
        }
        /* beads: appear from ~50, peak ~110, gone by ~300 */
        float benv = gk_env(age - 50.0f, 60.0f, 60.0f, 130.0f);
        if (benv > 0.0f) {
            float cool = gk_smooth((age - 110.0f) / 150.0f);   /* hue shift as they cool */
            for (int i = 0; i < b514[s].n; i += 5) {
                const gk_bseg *sg = &b514[s].s[i];
                if (sg->wgt < 0.45f) continue;                /* trunk only */
                uint32_t hh = (uint32_t)i * 131u + (uint32_t)idx * 17u + seed;
                float bs = 0.6f + 0.8f * gk_hash(hh);
                float ba = 0.5f + 0.5f * gk_hash(hh + 1u);
                float bh = gk_hash(hh + 2u);
                float ph2 = 0.85f + 0.15f * sinf(t * 0.02f + (float)i * 0.7f);   /* faint breath */
                int bpi = pi + (int)(bh * 1500.0f) + (int)(cool * 2500.0f);
                float bc[3], bhc[3];
                gk_col(pal, bpi, 0.5f * (1.0f - cool), 0.9f * benv * ba * ph2, bc);
                gk_col(pal, bpi + 800, 0.05f, 0.35f * benv * ba, bhc);
                gk_dot(&g514, sg->x1, sg->y1, bhc, 5.0f * sc * bs, 18.0f * sc * bs, 0.5f);
                gk_dot(&g514, sg->x1, sg->y1, bc, 2.4f * sc * bs, 7.0f * sc * bs, 0.5f);
            }
        }
    }
    gk_present(&g514, fb, w, h);
}
