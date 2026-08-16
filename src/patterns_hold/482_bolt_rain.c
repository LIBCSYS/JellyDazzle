/* 482 Bolt Rain — a whole sky of small thin bolts, thirty slots each on
 * its own long clock: a bolt grows down over ~30 frames, glows, and fades
 * over ~60, so at any moment a dozen are alive at different stages across
 * the frame, like a slow rain of sparks.  Field-density overlay with black
 * between.  Repaint pattern. */
#include "_hue469.h"

#define NS482 30
#define P482 260

static gk g482;
static gk_bolt b482[NS482];
static int bi482[NS482];
static uint32_t bs482 = 0xFFFFFFFFu;
static float hue482[NS482];

void pattern_482(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g482, w, h);
    gk_clear(&g482);
    if (seed != bs482) { for (int i = 0; i < NS482; i++) bi482[i] = -1; bs482 = seed; }
    float cw = (float)g482.cw, ch = (float)g482.ch, sc = g482.sc, t = (float)frame;
    int base = (int)(t * 2.4f) + (int)(seed & 8191u);
    for (int s = 0; s < NS482; s++) {
        int ph = frame + (s * 61) % P482;
        int idx = ph / P482;
        float age = (float)(ph - idx * P482);
        if (idx != bi482[s]) {
            gk_seed(&g482, seed ^ (uint32_t)(idx * 2003 + s * 5507));
            float x0 = cw * gk_rf(&g482), y0 = ch * (gk_rf(&g482) * 0.7f - 0.05f);
            float len = ch * (0.15f + 0.3f * gk_rf(&g482));
            gk_bolt_gen(&g482, &b482[s], x0, y0, x0 + len * 0.3f * gk_rs(&g482), y0 + len,
                        0.2f, 5, 2, 0.35f);
            hue482[s] = gk_rf(&g482);
            bi482[s] = idx;
        }
        float env = gk_env(age, 10.0f, 30.0f, 60.0f);
        if (env <= 0.0f) continue;
        int pi = base + (int)(hue482[s] * 6000.0f) + (int)(age * 10.0f);
        hk_style st;
        hk_style_set(&st, 3500, 1200, 700,
                     0.45f * env, 1.4f * sc, 5.0f * sc, 0.5f,
                     0.40f, 0.6f * env, 0.6f * sc, 1.6f * sc, 0.2f);
        hk_bolt(&g482, &b482[s], age / 30.0f, 1.0f, pal, pi, &st);
    }
    gk_present(&g482, fb, w, h);
}
