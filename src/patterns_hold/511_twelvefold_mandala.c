/* 511 Twelvefold Mandala — bolts strike INWARD from an invisible rim toward
 * the centre in twelve-fold rotational symmetry, so each strike closes like
 * an iris of lightning: three strike slots on staggered clocks, each rosette
 * growing inward over ~45 frames, holding, and dissolving over ~110 while
 * the fold turns very slowly.  Colour is a gradient along each bolt (rim
 * hue to centre hue), different per slot, drifting with time; the hub blooms
 * as the tips arrive.  Figure overlay, transparent outside.  Repaint. */
#include "_trace509.h"

#define P511 240
#define NS511 3

static gk g511;
static gk_bolt b511[NS511];
static int bi511[NS511] = { -1, -1, -1 };
static uint32_t bs511 = 0xFFFFFFFFu;
static float hue511[NS511], rad511[NS511], rot511[NS511];

void pattern_511(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g511, w, h);
    gk_clear(&g511);
    if (seed != bs511) { for (int s = 0; s < NS511; s++) bi511[s] = -1; bs511 = seed; }
    float cw = (float)g511.cw, ch = (float)g511.ch, sc = g511.sc, t = (float)frame;
    float cx = cw * 0.5f, cy = ch * 0.5f, rmax = (cw < ch ? cw : ch) * 0.48f;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float hub = 0.0f;
    for (int s = 0; s < NS511; s++) {
        int ph = frame + s * (P511 / NS511);
        int idx = ph / P511;
        float age = (float)(ph - idx * P511);
        if (idx != bi511[s]) {
            gk_seed(&g511, seed ^ (uint32_t)(idx * 5261 + s * 9151));
            /* local space: rim (1,0) to near-centre, curving with an off-axis end */
            gk_bolt_gen(&g511, &b511[s], 1.0f, 0.0f, 0.10f, 0.08f * gk_rs(&g511), 0.16f, 5, 3, 0.35f);
            hue511[s] = gk_rf(&g511);
            rad511[s] = 0.75f + 0.25f * gk_rf(&g511);
            rot511[s] = gk_rf(&g511) * GK_TAU;
            bi511[s] = idx;
        }
        float env = gk_env(age, 8.0f, 90.0f, 110.0f);
        if (env <= 0.0f) continue;
        float prog = age / 45.0f;
        int pi = base + (int)(hue511[s] * 8000.0f);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.40f * env, h0);            /* rim halo hue   */
        gk_col(pal, pi + 2600, 0.05f, 0.40f * env, h1);     /* centre halo    */
        gk_col(pal, pi + 400, 0.55f, 0.65f * env, c0);
        gk_col(pal, pi + 3000, 0.5f, 0.65f * env, c1);
        float rot = rot511[s] + t * 0.0009f * (s & 1 ? -1.0f : 1.0f);
        float scl = rmax * rad511[s];
        /* one capsule pass per copy (twelve copies x three slots adds up): core + halo folded */
        float m0[3] = { c0[0] * 0.8f + h0[0] * 0.5f, c0[1] * 0.8f + h0[1] * 0.5f, c0[2] * 0.8f + h0[2] * 0.5f };
        float m1[3] = { c1[0] * 0.8f + h1[0] * 0.5f, c1[1] * 0.8f + h1[1] * 0.5f, c1[2] * 0.8f + h1[2] * 0.5f };
        for (int k = 0; k < 12; k++) {
            float r = rot + GK_TAU * (float)k / 12.0f;
            bx_draw_grad(&g511, &b511[s], cx, cy, r, scl, 1.0f, prog, 1.0f, m0, m1, 0.0f, 1.0f * sc, 4.5f * sc, 0.55f);
        }
        hub += env * gk_smooth(prog - 0.7f);
    }
    if (hub > 0.0f) {
        float hc[3];
        gk_col(pal, base + 3500, 0.5f, 0.9f * (hub > 1.5f ? 1.5f : hub), hc);
        gk_dot(&g511, cx, cy, hc, 3.0f * sc, 16.0f * sc, 0.6f);
    }
    gk_present(&g511, fb, w, h);
}
