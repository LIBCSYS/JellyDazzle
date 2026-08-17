/* 516 Skyline Leaders — a city skyline in faint outline (a row of tower
 * silhouettes of varied height along the bottom), and from the rooftops
 * upward positive leaders reach for the sky: six leader slots on staggered
 * clocks, each a thin branching discharge that climbs from a roof corner
 * over ~35 frames, wavers, and withdraws over ~70, its root hue on the
 * roof shifting to another hue at the tips.  Rooftops glow where leaders
 * stand.  Sparse-to-figure overlay; sky is black.  Repaint. */
#include "_trace509.h"

#define NB516 11
#define NL516 6
#define P516 210

static gk g516;
static gk_bolt b516[NL516];
static int bi516[NL516] = { -1, -1, -1, -1, -1, -1 };
static uint32_t bs516 = 0xFFFFFFFFu;
static float rx516[NL516], ry516[NL516], hue516[NL516], lean516[NL516];
static float bx516[NB516], bw516[NB516], bh516[NB516];

void pattern_516(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g516, w, h);
    gk_clear(&g516);
    float cw = (float)g516.cw, ch = (float)g516.ch, sc = g516.sc, t = (float)frame;
    if (seed != bs516) {
        for (int s = 0; s < NL516; s++) bi516[s] = -1;
        gk_seed(&g516, seed ^ 0x516u);
        float x = 0.0f;
        for (int b = 0; b < NB516; b++) {
            bw516[b] = cw * (0.05f + 0.06f * gk_rf(&g516));
            bx516[b] = x; x += bw516[b] + cw * 0.01f * gk_rf(&g516);
            bh516[b] = ch * (0.12f + 0.32f * gk_rf(&g516) * gk_rf(&g516) + 0.1f * gk_rf(&g516));
        }
        float k = cw / x;                       /* fit the row to the width */
        for (int b = 0; b < NB516; b++) { bx516[b] *= k; bw516[b] *= k; }
        bs516 = seed;
    }
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float gy = ch * 0.96f;
    /* skyline outline: faint, hue drifting */
    float oc[3];
    gk_col(pal, base + 4500, 0.1f, 0.10f, oc);
    for (int b = 0; b < NB516; b++) {
        float x0 = bx516[b], x1 = bx516[b] + bw516[b], y0 = gy - bh516[b];
        gk_seg(&g516, x0, gy, x0, y0, oc, 0.7f * sc, 2.0f * sc, 0.3f);
        gk_seg(&g516, x0, y0, x1, y0, oc, 0.7f * sc, 2.0f * sc, 0.3f);
        gk_seg(&g516, x1, y0, x1, gy, oc, 0.7f * sc, 2.0f * sc, 0.3f);
    }
    gk_seg(&g516, 0.0f, gy, cw, gy, oc, 0.8f * sc, 2.5f * sc, 0.3f);
    for (int s = 0; s < NL516; s++) {
        int ph = frame + s * (P516 / NL516) + s * 13;
        int idx = ph / P516;
        float age = (float)(ph - idx * P516);
        if (idx != bi516[s]) {
            gk_seed(&g516, seed ^ (uint32_t)(idx * 4483 + s * 9337));
            int b = (int)(gk_rf(&g516) * (float)NB516) % NB516;
            rx516[s] = bx516[b] + (gk_rf(&g516) < 0.5f ? 0.0f : bw516[b]);
            ry516[s] = gy - bh516[b];
            hue516[s] = gk_rf(&g516);
            lean516[s] = 0.35f * gk_rs(&g516);
            float len = ch * (0.25f + 0.3f * gk_rf(&g516));
            gk_bolt_gen(&g516, &b516[s], 0.0f, 0.0f, lean516[s] * len, -len, 0.17f, 6, 4, 0.4f);
            bi516[s] = idx;
        }
        float env = gk_env(age, 10.0f, 50.0f, 70.0f);
        if (env <= 0.0f) continue;
        float prog = age / 35.0f;
        int pi = base + (int)(hue516[s] * 8000.0f);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.45f * env, h0);
        gk_col(pal, pi + 1600, 0.05f, 0.30f * env, h1);
        gk_col(pal, pi + 300, 0.55f, 0.7f * env, c0);
        gk_col(pal, pi + 1900, 0.35f, 0.55f * env, c1);
        float sway = 0.03f * sinf(t * 0.013f + (float)s);
        bx_draw_grad(&g516, &b516[s], rx516[s], ry516[s], sway, 1.0f, 1.0f, prog, 1.0f, h0, h1, 0.3f, 1.8f * sc, 6.0f * sc, 0.5f);
        bx_draw_grad(&g516, &b516[s], rx516[s], ry516[s], sway, 1.0f, 1.0f, prog, 1.0f, c0, c1, 0.3f, 0.8f * sc, 2.0f * sc, 0.25f);
        float rc[3];
        gk_col(pal, pi + 600, 0.3f, 0.6f * env, rc);
        gk_dot(&g516, rx516[s], ry516[s], rc, 3.0f * sc, 14.0f * sc, 0.6f);
    }
    gk_present(&g516, fb, w, h);
}
