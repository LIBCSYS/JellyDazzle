/* 469 Forked Bolt — one great forked bolt at a time, sky to ground.
 * Midpoint-displacement channel with recursive branches; each strike grows
 * tip-first over ~28 frames, holds with a slow breathing halo, then fades
 * over ~70 frames.  Two overlapping strike slots so a new bolt is already
 * reaching down while the last one dies.  Transparent (black) everywhere
 * else — a figure/spark overlay.  Repaint pattern. */
#include "_hue469.h"

static gk g469;
static gk_bolt b469[2];
static int  bi469[2] = { -1, -1 };
static uint32_t bseed469[2] = { 0, 0 };
static float bcol469[2];

#define P469 210          /* frames between strikes per slot */

static void gen469(int slot, int idx, uint32_t seed)
{
    gk_seed(&g469, seed ^ (uint32_t)(idx * 7919 + slot * 104729));
    float cw = (float)g469.cw, ch = (float)g469.ch;
    float x0 = cw * (0.2f + 0.6f * gk_rf(&g469));
    float x1 = x0 + cw * 0.35f * gk_rs(&g469);
    gk_bolt_gen(&g469, &b469[slot], x0, -ch * 0.02f, x1, ch * (0.85f + 0.15f * gk_rf(&g469)),
                0.22f, 7, 7, 0.45f);
    bcol469[slot] = gk_rf(&g469);
    bi469[slot] = idx; bseed469[slot] = seed;
}

void pattern_469(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g469, w, h);
    gk_clear(&g469);
    float sc = g469.sc;
    int base = (int)(frame * 3.1f) + (int)(seed & 4095u);
    for (int slot = 0; slot < 2; slot++) {
        int ph = frame + slot * (P469 / 2);
        int idx = ph / P469;
        float age = (float)(ph - idx * P469);
        if (idx != bi469[slot] || seed != bseed469[slot]) gen469(slot, idx, seed);
        float grow = age / 28.0f;
        float env = gk_env(age, 10.0f, 30.0f, 70.0f);
        if (env <= 0.0f) continue;
        float breathe = 0.85f + 0.15f * sinf(age * 0.11f);
        /* hue slides along the channel (pspan), steps at each fork (bspan)
         * and drifts with age so the bolt morphs while it burns */
        int pi = base + (int)(bcol469[slot] * 9000.0f) + (int)(age * 14.0f);
        hk_style st;
        hk_style_set(&st, 6000, 2200, 900,
                     0.9f * env * breathe, 2.6f * sc, 9.0f * sc, 0.55f,
                     0.42f, 1.5f * env, 1.1f * sc, 3.0f * sc, 0.25f);
        hk_bolt(&g469, &b469[slot], grow, 1.0f, pal, pi, &st);
    }
    gk_present(&g469, fb, w, h);
}
