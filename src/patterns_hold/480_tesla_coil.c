/* 480 Tesla Coil — a toroid at the bottom centre throws a slow fan of
 * streamers upward: eight streamer slots, each a thin branching discharge
 * that reaches up and out over ~35 frames, wavers, and withdraws, so the
 * crown of the coil is always alive but nothing snaps.  A soft corona sits
 * on the toroid.  Sparse-to-figure overlay.  Repaint pattern. */
#include "_hue469.h"

#define NS480 8
#define P480 120

static gk g480;
static gk_bolt b480[NS480];
static int bi480[NS480];
static uint32_t bs480 = 0xFFFFFFFFu;
static float ang480[NS480], hue480[NS480];

void pattern_480(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g480, w, h);
    gk_clear(&g480);
    if (seed != bs480) { for (int i = 0; i < NS480; i++) bi480[i] = -1; bs480 = seed; }
    float cw = (float)g480.cw, ch = (float)g480.ch, sc = g480.sc, t = (float)frame;
    float cx = cw * 0.5f, cy = ch * 0.86f;
    float R = (cw < ch ? cw : ch) * 0.75f;
    int base = (int)(t * 2.0f) + (int)(seed & 8191u);
    /* toroid corona */
    float cor[3], hot[3];
    gk_col(pal, base + 4000, 0.15f, 0.5f + 0.1f * sinf(t * 0.03f), cor);
    gk_col(pal, base + 4000, 0.7f, 0.9f, hot);
    gk_ring(&g480, cx, cy, 30.0f * sc, 9.0f * sc, cor);
    gk_seg(&g480, cx - 26.0f * sc, cy, cx + 26.0f * sc, cy, hot, 2.0f * sc, 8.0f * sc, 0.5f);
    for (int s = 0; s < NS480; s++) {
        int ph = frame + (s * 47) % P480;
        int idx = ph / P480;
        float age = (float)(ph - idx * P480);
        if (idx != bi480[s]) {
            gk_seed(&g480, seed ^ (uint32_t)(idx * 4093 + s * 7001));
            ang480[s] = -GK_TAU * 0.25f + gk_rs(&g480) * 1.25f;   /* fan: up +-72deg */
            float len = 0.45f + 0.55f * gk_rf(&g480);
            /* local space: origin at coil, unit = R */
            gk_bolt_gen(&g480, &b480[s], cosf(ang480[s]) * 0.04f, sinf(ang480[s]) * 0.04f,
                        cosf(ang480[s]) * len, sinf(ang480[s]) * len, 0.2f, 6, 3, 0.4f);
            hue480[s] = gk_rf(&g480);
            bi480[s] = idx;
        }
        float env = gk_env(age, 8.0f, 30.0f, 45.0f);
        if (env <= 0.0f) continue;
        float sway = 0.04f * sinf(t * 0.02f + (float)s);
        int pi = base + (int)(hue480[s] * 5000.0f) + (int)(age * 12.0f);
        hk_style st;
        hk_style_set(&st, 4500, 1600, 800,
                     0.5f * env, 1.6f * sc, 6.0f * sc, 0.5f,
                     0.40f, 0.65f * env, 0.7f * sc, 2.0f * sc, 0.2f);
        hk_bolt_xf(&g480, &b480[s], cx, cy, sway, R, age / 35.0f, 1.0f, pal, pi, &st);
    }
    gk_present(&g480, fb, w, h);
}
