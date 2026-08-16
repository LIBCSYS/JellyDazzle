/* 478 Crawler Arcs — anvil crawlers: long, nearly horizontal discharges
 * that creep across the top of the frame under an unseen cloud base, each
 * one a slow leader that travels left-to-right (or back) over ~60 frames,
 * throwing short branches downward, then dims behind its own tip.  Three
 * crawlers on staggered clocks.  Sparse overlay across the upper third.
 * Repaint pattern. */
#include "_hue469.h"

#define NS478 3
#define P478 240

static gk g478;
static gk_bolt b478[NS478];
static int bi478[NS478] = { -1, -1, -1 };
static uint32_t bs478 = 0xFFFFFFFFu;
static float hue478[NS478];

void pattern_478(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g478, w, h);
    gk_clear(&g478);
    if (seed != bs478) { for (int i = 0; i < NS478; i++) bi478[i] = -1; bs478 = seed; }
    float cw = (float)g478.cw, ch = (float)g478.ch, sc = g478.sc, t = (float)frame;
    int base = (int)(t * 2.2f) + (int)(seed & 8191u);
    for (int s = 0; s < NS478; s++) {
        int ph = frame + s * (P478 / NS478);
        int idx = ph / P478;
        float age = (float)(ph - idx * P478);
        if (idx != bi478[s]) {
            gk_seed(&g478, seed ^ (uint32_t)(idx * 3571 + s * 12007));
            int dir = gk_rf(&g478) < 0.5f ? 1 : -1;
            float x0 = dir > 0 ? -cw * 0.05f : cw * 1.05f;
            float x1 = x0 + (float)dir * cw * (0.6f + 0.5f * gk_rf(&g478));
            float y0 = ch * (0.08f + 0.3f * gk_rf(&g478));
            float y1 = y0 + ch * 0.15f * gk_rs(&g478);
            gk_bolt_gen(&g478, &b478[s], x0, y0, x1, y1, 0.16f, 7, 6, 0.3f);
            hue478[s] = gk_rf(&g478);
            bi478[s] = idx;
        }
        /* the crawler: reveal window — bright at the tip, dimming behind */
        float prog = age / 60.0f;
        float env = gk_env(age, 6.0f, 70.0f, 70.0f);
        if (env <= 0.0f) continue;
        int pi = base + (int)(hue478[s] * 8000.0f) + (int)(age * 9.0f);
        float c[3];
        /* body: hue slides along the crawler and steps at each branch;
         * then the head 25% gets an extra bright pass */
        hk_style st;
        hk_style_set(&st, 6000, 2000, 900,
                     0.5f * env, 2.0f * sc, 7.0f * sc, 0.5f,
                     0.40f, 0.65f * env, 0.8f * sc, 2.2f * sc, 0.2f);
        hk_bolt(&g478, &b478[s], prog, 1.0f, pal, pi, &st);
        hk_col(pal, pi + (int)(prog * 6000.0f), 0.30f, 0.45f * env, c);
        /* head brightening: segments within the last 0.2 of prog get extra */
        for (int i = 0; i < b478[s].n; i++) {
            const gk_bseg *sg = &b478[s].s[i];
            if (sg->t0 >= prog || sg->t1 < prog - 0.25f) continue;
            float k = 1.0f - (prog - sg->t0) / 0.25f; if (k < 0.0f) k = 0.0f; if (k > 1.0f) k = 1.0f;
            float x1 = sg->x1, y1 = sg->y1;
            if (sg->t1 > prog) { float f = (prog - sg->t0) / (sg->t1 - sg->t0); x1 = sg->x0 + (sg->x1 - sg->x0) * f; y1 = sg->y0 + (sg->y1 - sg->y0) * f; }
            float hc2[3] = { c[0] * k * sg->wgt, c[1] * k * sg->wgt, c[2] * k * sg->wgt };
            gk_seg(&g478, sg->x0, sg->y0, x1, y1, hc2, 1.4f * sc, 5.0f * sc, 0.5f);
        }
    }
    gk_present(&g478, fb, w, h);
}
