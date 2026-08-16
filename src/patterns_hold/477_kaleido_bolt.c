/* 477 Kaleido Bolt — a single wandering bolt seen through a six-mirror
 * kaleidoscope: it strikes from an off-centre point toward the rim, and the
 * fold turns it into a snowflake of lightning.  One long-lived bolt at a
 * time (grows ~40 frames, holds ~90, fades ~90) while the fold rotates
 * slowly, plus a second dim one out of phase.  Repaint; centre-weighted
 * figure, transparent toward the corners. */
#include "_hue469.h"

#define P477 300

static gk g477;
static gk_bolt b477[2];
static int bi477[2] = { -1, -1 };
static uint32_t bs477 = 0xFFFFFFFFu;
static float hue477[2];

void pattern_477(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g477, w, h);
    gk_clear(&g477);
    if (seed != bs477) { bi477[0] = bi477[1] = -1; bs477 = seed; }
    float cw = (float)g477.cw, ch = (float)g477.ch, sc = g477.sc, t = (float)frame;
    float cx = cw * 0.5f, cy = ch * 0.5f;
    float rmax = (cw < ch ? cw : ch) * 0.5f;
    int base = (int)(t * 1.7f) + (int)(seed & 8191u);
    for (int s = 0; s < 2; s++) {
        int ph = frame + s * (P477 / 2);
        int idx = ph / P477;
        float age = (float)(ph - idx * P477);
        if (idx != bi477[s]) {
            gk_seed(&g477, seed ^ (uint32_t)(idx * 5443 + s * 9973));
            /* start somewhere inside the wedge, head outward-ish */
            float a0 = gk_rf(&g477) * 0.5f, r0 = 0.1f + 0.35f * gk_rf(&g477);
            float a1 = a0 + gk_rs(&g477) * 0.7f, r1 = 0.85f + 0.2f * gk_rf(&g477);
            gk_bolt_gen(&g477, &b477[s], cosf(a0) * r0, sinf(a0) * r0, cosf(a1) * r1, sinf(a1) * r1,
                        0.24f, 6, 4, 0.45f);
            hue477[s] = gk_rf(&g477);
            bi477[s] = idx;
        }
        float env = gk_env(age, 12.0f, 90.0f, 90.0f) * (s ? 0.55f : 1.0f);
        if (env <= 0.0f) continue;
        float rot = t * 0.0011f;
        int pi = base + (int)(hue477[s] * 8000.0f) + (int)(age * 8.0f);
        hk_style st;
        hk_style_set(&st, 5500, 1800, 1100,
                     0.4f * env, 2.0f * sc, 7.0f * sc, 0.5f,
                     0.40f, 0.75f * env, 0.9f * sc, 2.4f * sc, 0.25f);
        hk_kaleido(&g477, &b477[s], cx, cy, rot, rmax, 6, 1, age / 40.0f, 1.0f, pal, pi, &st);
    }
    gk_present(&g477, fb, w, h);
}
