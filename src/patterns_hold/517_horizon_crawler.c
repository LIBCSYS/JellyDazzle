/* 517 Horizon Crawler — a low, wide view: the horizon sits at 60% and just
 * above it long flat discharges crawl sideways along the far cloud base,
 * each a slow leader travelling left-to-right or back over ~70 frames,
 * throwing short branches, then dimming behind its tip; the ground below
 * the horizon catches a soft reflected wash under each crawler.  Three
 * crawler slots, each its own hue, hue drifting along the channel.  Field
 * across the middle band; sky and far ground dark.  Repaint. */
#include "_trace509.h"

#define NC517 3
#define P517 260

static gk g517;
static gk_bolt b517[NC517];
static int bi517[NC517] = { -1, -1, -1 };
static uint32_t bs517 = 0xFFFFFFFFu;
static float x0517[NC517], x1517[NC517], y517[NC517], hue517[NC517];

void pattern_517(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g517, w, h);
    gk_clear(&g517);
    if (seed != bs517) { for (int s = 0; s < NC517; s++) bi517[s] = -1; bs517 = seed; }
    float cw = (float)g517.cw, ch = (float)g517.ch, sc = g517.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float hy = ch * 0.60f;
    /* horizon line, faint */
    float hc[3];
    gk_col(pal, base + 4000, 0.15f, 0.08f, hc);
    gk_seg(&g517, 0.0f, hy, cw, hy, hc, 1.0f * sc, 4.0f * sc, 0.4f);
    for (int s = 0; s < NC517; s++) {
        int ph = frame + s * (P517 / NC517) + s * 7;
        int idx = ph / P517;
        float age = (float)(ph - idx * P517);
        if (idx != bi517[s]) {
            gk_seed(&g517, seed ^ (uint32_t)(idx * 2707 + s * 5843));
            float span = cw * (0.3f + 0.45f * gk_rf(&g517));
            float xa = gk_rf(&g517) * (cw - span);
            if (gk_rf(&g517) < 0.5f) { x0517[s] = xa; x1517[s] = xa + span; } else { x0517[s] = xa + span; x1517[s] = xa; }
            y517[s] = hy - ch * (0.02f + 0.10f * gk_rf(&g517));
            hue517[s] = gk_rf(&g517);
            gk_bolt_gen(&g517, &b517[s], 0.0f, 0.0f, 1.0f, 0.0f, 0.07f, 7, 5, 0.22f);
            bi517[s] = idx;
        }
        float env = gk_env(age, 10.0f, 70.0f, 80.0f);
        if (env <= 0.0f) continue;
        float prog = age / 70.0f;
        float len = x1517[s] - x0517[s];
        float ang = len < 0.0f ? GK_TAU * 0.5f : 0.0f;
        len = fabsf(len);
        int pi = base + (int)(hue517[s] * 8000.0f);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.45f * env, h0);
        gk_col(pal, pi + 2000, 0.05f, 0.40f * env, h1);
        gk_col(pal, pi + 300, 0.55f, 0.7f * env, c0);
        gk_col(pal, pi + 2300, 0.45f, 0.65f * env, c1);
        /* dim behind the tip: fade older parts once the leader has passed */
        float tail = gk_smooth((prog - 1.0f) / 0.6f);
        bx_draw_grad(&g517, &b517[s], x0517[s], y517[s], ang, len, 1.0f, prog, 1.0f - 0.5f * tail, h0, h1, 0.0f, 1.6f * sc, 6.0f * sc, 0.5f);
        bx_draw_grad(&g517, &b517[s], x0517[s], y517[s], ang, len, 1.0f, prog, 1.0f - 0.5f * tail, c0, c1, 0.0f, 0.7f * sc, 1.9f * sc, 0.25f);
        /* tip */
        if (prog < 1.0f) {
            float tc[3];
            gk_col(pal, pi + 1000, 0.6f, 0.9f * env, tc);
            gk_dot(&g517, x0517[s] + cosf(ang) * len * prog, y517[s], tc, 1.8f * sc, 8.0f * sc, 0.6f);
        }
        /* reflection below the horizon: the same channel mirrored, blurred, dim */
        float r0[3], r1[3];
        gk_col(pal, pi + 600, 0.05f, 0.16f * env, r0);
        gk_col(pal, pi + 2600, 0.05f, 0.14f * env, r1);
        bx_draw_grad(&g517, &b517[s], x0517[s], 2.0f * hy - y517[s], -ang, len, -1.0f, prog, 1.0f - 0.5f * tail, r0, r1, 0.0f, 4.0f * sc, 14.0f * sc, 0.9f);
    }
    gk_present(&g517, fb, w, h);
}
