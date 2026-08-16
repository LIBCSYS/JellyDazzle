/* 533 Lightning Echo — a bolt strikes (growing over ~22 frames) and then
 * echoes: ghost copies of the same channel peel away to left and right,
 * each pair launched a couple of dozen frames after the last, sliding
 * outward at under a pixel a frame, wider and softer and dimmer the
 * further they go, each echo shifted to its own palette hue, while the
 * original cools between them.  Two strike slots on staggered clocks.
 * Sparse-to-figure overlay.  Repaint. */
#include "_trace509.h"

#define P533 330
#define NE533 3

static gk g533;
static gk_bolt b533[2];
static int bi533[2] = { -1, -1 };
static uint32_t bs533 = 0xFFFFFFFFu;
static float hue533[2];

void pattern_533(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g533, w, h);
    gk_clear(&g533);
    if (seed != bs533) { bi533[0] = bi533[1] = -1; bs533 = seed; }
    float cw = (float)g533.cw, ch = (float)g533.ch, sc = g533.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    for (int s = 0; s < 2; s++) {
        int ph = frame + s * (P533 / 2);
        int idx = ph / P533;
        float age = (float)(ph - idx * P533);
        if (idx != bi533[s]) {
            gk_seed(&g533, seed ^ (uint32_t)(idx * 5081 + s * 6733));
            float x0 = cw * (0.3f + 0.4f * gk_rf(&g533));
            gk_bolt_gen(&g533, &b533[s], x0, -ch * 0.02f, x0 + cw * 0.15f * gk_rs(&g533), ch * (0.85f + 0.12f * gk_rf(&g533)), 0.18f, 6, 4, 0.35f);
            hue533[s] = gk_rf(&g533);
            bi533[s] = idx;
        }
        int pi = base + (int)(hue533[s] * 8000.0f);
        /* the original */
        float env = gk_env(age, 8.0f, 50.0f, 120.0f);
        if (env > 0.0f) {
            float c0[3], c1[3], h0[3], h1[3];
            gk_col(pal, pi, 0.05f, 0.45f * env, h0);
            gk_col(pal, pi + 1000, 0.05f, 0.40f * env, h1);
            gk_col(pal, pi + 200, 0.6f, 0.7f * env, c0);
            gk_col(pal, pi + 1200, 0.5f, 0.65f * env, c1);
            bx_draw_grad(&g533, &b533[s], 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, age / 22.0f, 1.0f, h0, h1, 0.1f, 2.0f * sc, 7.0f * sc, 0.5f);
            bx_draw_grad(&g533, &b533[s], 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, age / 22.0f, 1.0f, c0, c1, 0.1f, 0.9f * sc, 2.4f * sc, 0.25f);
        }
        /* echoes */
        for (int e = 0; e < NE533; e++) {
            float ea = age - 30.0f - 26.0f * (float)e;
            if (ea < 0.0f) continue;
            float eenv = gk_env(ea, 30.0f, 40.0f, 130.0f) * (0.7f - 0.15f * (float)e);
            if (eenv <= 0.0f) continue;
            float off = ea * (0.55f + 0.15f * (float)e) * sc;
            float wide = 1.0f + 0.005f * ea;
            int pj = pi + 1800 + e * 1400;
            float c0[3], c1[3], h0[3], h1[3];
            gk_col(pal, pj, 0.05f, 0.30f * eenv, h0);
            gk_col(pal, pj + 900, 0.05f, 0.25f * eenv, h1);
            gk_col(pal, pj + 300, 0.3f, 0.45f * eenv, c0);
            gk_col(pal, pj + 1200, 0.25f, 0.4f * eenv, c1);
            /* ghosts: one soft pass each side, core+halo folded into one capsule */
            float g0[3] = { h0[0] + c0[0] * 0.6f, h0[1] + c0[1] * 0.6f, h0[2] + c0[2] * 0.6f };
            float g1[3] = { h1[0] + c1[0] * 0.6f, h1[1] + c1[1] * 0.6f, h1[2] + c1[2] * 0.6f };
            for (int d = -1; d <= 1; d += 2)
                bx_draw_grad(&g533, &b533[s], off * (float)d, 0.0f, 0.0f, 1.0f, 1.0f, 2.0f, 1.0f, g0, g1, 0.1f, 1.6f * sc * wide, 5.0f * sc * wide, 0.5f);
        }
    }
    gk_present(&g533, fb, w, h);
}
