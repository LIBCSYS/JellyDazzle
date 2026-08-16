/* 476 Lightning Mandala — bolts leave the centre outward in eight-fold
 * mirrored symmetry, so every strike blossoms as a spiked rosette.  Three
 * strike slots on staggered clocks; each rosette grows out over ~30 frames,
 * turns very slowly, and dims away over ~80.  A soft hub glow sits at the
 * centre.  Repaint pattern; transparent outside the rosette. */
#include "_hue469.h"

#define NS476 3
#define P476 200

static gk g476;
static gk_bolt b476[NS476];
static int bi476[NS476] = { -1, -1, -1 };
static uint32_t bs476 = 0xFFFFFFFFu;
static float hue476[NS476], rad476[NS476];
static int fold476[NS476];

void pattern_476(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g476, w, h);
    gk_clear(&g476);
    if (seed != bs476) { for (int i = 0; i < NS476; i++) bi476[i] = -1; bs476 = seed; }
    float cw = (float)g476.cw, ch = (float)g476.ch, sc = g476.sc, t = (float)frame;
    float cx = cw * 0.5f, cy = ch * 0.5f;
    float rmax = (cw < ch ? cw : ch) * 0.48f;
    int base = (int)(t * 2.0f) + (int)(seed & 8191u);
    for (int s = 0; s < NS476; s++) {
        int ph = frame + s * (P476 / NS476);
        int idx = ph / P476;
        float age = (float)(ph - idx * P476);
        if (idx != bi476[s]) {
            gk_seed(&g476, seed ^ (uint32_t)(idx * 8161 + s * 20011));
            rad476[s] = 0.55f + 0.45f * gk_rf(&g476);
            float a = 0.15f + 0.5f * gk_rf(&g476);
            gk_bolt_gen(&g476, &b476[s], 0.0f, 0.06f, cosf(a) * rad476[s], sinf(a) * rad476[s] + 0.06f,
                        0.22f, 5, 3, 0.4f);
            hue476[s] = gk_rf(&g476);
            fold476[s] = 5 + (int)(gk_rf(&g476) * 4.0f);   /* 5..8 */
            bi476[s] = idx;
        }
        float env = gk_env(age, 10.0f, 40.0f, 80.0f);
        if (env <= 0.0f) continue;
        float rot = t * 0.0015f * (s & 1 ? -1.0f : 1.0f) + (float)s;
        /* hue runs hub -> rim along every spoke and drifts with age */
        int pi = base + (int)(hue476[s] * 7000.0f) + (int)(age * 10.0f);
        hk_style st;
        hk_style_set(&st, 5000, 1800, 1000,
                     0.45f * env, 2.0f * sc, 6.0f * sc, 0.5f,
                     0.40f, 0.85f * env, 0.9f * sc, 2.4f * sc, 0.25f);
        hk_kaleido(&g476, &b476[s], cx, cy, rot, rmax, fold476[s], 1, age / 30.0f, 1.0f, pal, pi, &st);
    }
    float hub[3];
    gk_col(pal, base + 3000, 0.4f, 1.0f + 0.2f * sinf(t * 0.02f), hub);
    gk_dot(&g476, cx, cy, hub, 4.0f * sc, 22.0f * sc, 0.5f);
    gk_present(&g476, fb, w, h);
}
