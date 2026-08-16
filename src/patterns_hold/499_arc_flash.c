/* 499 Arc Flash — big, slow, close: a single massive bolt fills the frame
 * from top to bottom, thick channel, wide soft halo that washes the whole
 * area around it, growing over ~40 frames, holding with a slow shimmer for
 * ~150, and fading over ~130.  Between strikes the frame is dark; while
 * one burns, half the frame is lit.  Field-density overlay.  Repaint. */
#include "_hue469.h"

#define P499 420

static gk g499;
static gk_bolt b499;
static int bi499 = -1;
static uint32_t bs499 = 0xFFFFFFFFu;
static float hue499;

void pattern_499(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g499, w, h);
    gk_clear(&g499);
    if (seed != bs499) { bi499 = -1; bs499 = seed; }
    float cw = (float)g499.cw, ch = (float)g499.ch, sc = g499.sc, t = (float)frame;
    int idx = frame / P499;
    float age = (float)(frame - idx * P499);
    if (idx != bi499) {
        gk_seed(&g499, seed ^ (uint32_t)(idx * 7211));
        float x0 = cw * (0.25f + 0.5f * gk_rf(&g499));
        gk_bolt_gen(&g499, &b499, x0, -ch * 0.05f, x0 + cw * 0.3f * gk_rs(&g499), ch * 1.05f, 0.2f, 8, 8, 0.5f);
        hue499 = gk_rf(&g499);
        bi499 = idx;
    }
    float env = gk_env(age, 14.0f, 150.0f, 130.0f);
    if (env > 0.0f) {
        int base = (int)(t * 1.2f) + (int)(seed & 8191u) + (int)(hue499 * 7000.0f);
        float shimmer = 0.9f + 0.1f * sinf(age * 0.07f) * sinf(age * 0.031f + 1.0f);
        float prog = age / 40.0f;
        float wc[3];
        base += (int)(age * 8.0f);                                        /* life-morph */
        hk_style st;
        hk_style_set(&st, 7000, 2200, 800,
                     0.45f * env * shimmer, 2.6f * sc, 9.0f * sc, 0.5f,
                     0.42f, 1.15f * env, 1.1f * sc, 3.2f * sc, 0.25f);
        /* the wide wash only along the main channel, and only every 4th
         * segment (they overlap heavily at that radius anyway) */
        for (int i = 0; i < b499.n; i++) {
            const gk_bseg *s = &b499.s[i];
            if (s->wgt < 0.9f || (i & 3) || s->t0 >= prog) continue;
            gk_col(pal, base + 1500 + (int)(s->t0 * 7000.0f), 0.0f, 0.10f * env * shimmer, wc);
            gk_seg(&g499, s->x0, s->y0, s->x1, s->y1, wc, 6.0f * sc, 22.0f * sc, 1.0f);
        }
        hk_bolt(&g499, &b499, prog, 1.0f, pal, base, &st);
    }
    gk_present(&g499, fb, w, h);
}
