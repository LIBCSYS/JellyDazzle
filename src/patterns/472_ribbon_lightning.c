/* 472 Ribbon Lightning — a bolt whose channel is blown sideways by the wind
 * while it burns, so successive strokes lay down a translucent ribbon of
 * fading copies beside the live channel.  Persistence canvas with decay;
 * the bolt itself grows in over ~30 frames.  Repaint-with-memory pattern
 * (self-clearing through decay; nothing pops).  Per-frame amplitudes are
 * small because the persistence sums ~14 frames of copies. */
#include "_hue469.h"

static gk g472;
static gk_bolt b472[2];
static int bi472[2] = { -1, -1 };
static uint32_t bs472;
static float hue472[2], drift472[2];

#define P472 260

static void gen472(int slot, int idx, uint32_t seed)
{
    gk_seed(&g472, seed ^ (uint32_t)(idx * 6151 + slot * 30011));
    float cw = (float)g472.cw, ch = (float)g472.ch;
    float x0 = cw * (0.25f + 0.5f * gk_rf(&g472));
    gk_bolt_gen(&g472, &b472[slot], x0, -ch * 0.03f, x0 + cw * 0.2f * gk_rs(&g472),
                ch * (0.8f + 0.2f * gk_rf(&g472)), 0.13f, 7, 4, 0.4f);
    hue472[slot] = gk_rf(&g472);
    drift472[slot] = (gk_rf(&g472) < 0.5f ? -1.0f : 1.0f) * (0.6f + 0.6f * gk_rf(&g472));
    bi472[slot] = idx;
}

void pattern_472(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g472, w, h);
    if (seed != bs472) { bi472[0] = bi472[1] = -1; bs472 = seed; }
    gk_decay_snap(&g472, 0.93f);
    float sc = g472.sc;
    int base = (int)(frame * 2.7f) + (int)(seed & 8191u);
    for (int slot = 0; slot < 2; slot++) {
        int ph = frame + slot * (P472 / 2);
        int idx = ph / P472;
        float age = (float)(ph - idx * P472);
        if (idx != bi472[slot]) gen472(slot, idx, seed);
        float env = gk_env(age, 8.0f, 60.0f, 50.0f);
        if (env <= 0.0f) continue;
        float ox = drift472[slot] * sc * age * 0.4f;
        /* hue runs down the channel and drifts with age, so the ribbon of
         * fading copies is a colour gradient in both directions */
        int pi = base + (int)(hue472[slot] * 8000.0f) + (int)(age * 10.0f);
        hk_style st;
        hk_style_set(&st, 5500, 2000, 1200,
                     0.09f * env, 2.4f * sc, 8.0f * sc, 0.5f,
                     0.40f, 0.24f * env, 1.0f * sc, 2.6f * sc, 0.2f);
        hk_bolt_xf(&g472, &b472[slot], ox, 0.0f, 0.0f, 1.0f, age / 30.0f, 1.0f, pal, pi, &st);
    }
    gk_present(&g472, fb, w, h);
}
