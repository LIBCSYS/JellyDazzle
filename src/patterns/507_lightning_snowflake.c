/* 507 Lightning Snowflake — six-fold mirrored dendrite: one heavily
 * branched bolt grows from the centre outward over ~300 frames, its twelve
 * reflected copies making a crystal that keeps sprouting new arms as the
 * leader advances; it holds, breathing, then dissolves while the next
 * flake (new geometry, new hue) begins.  Figure overlay.  Repaint. */
#include "_hue469.h"

#define P507 620

static gk g507;
static gk_bolt b507[2];
static int bi507[2] = { -1, -1 };
static uint32_t bs507 = 0xFFFFFFFFu;
static float hue507[2];

void pattern_507(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g507, w, h);
    gk_clear(&g507);
    if (seed != bs507) { bi507[0] = bi507[1] = -1; bs507 = seed; }
    float cw = (float)g507.cw, ch = (float)g507.ch, sc = g507.sc, t = (float)frame;
    float cx = cw * 0.5f, cy = ch * 0.5f, rmax = (cw < ch ? cw : ch) * 0.47f;
    int base = (int)(t * 1.1f) + (int)(seed & 8191u);
    for (int s = 0; s < 2; s++) {
        int ph = frame + s * (P507 / 2);
        int idx = ph / P507;
        float age = (float)(ph - idx * P507);
        if (idx != bi507[s]) {
            gk_seed(&g507, seed ^ (uint32_t)(idx * 6763 + s * 12007));
            float a = 0.05f + 0.42f * gk_rf(&g507);       /* inside a 60-degree wedge (mirrored) */
            gk_bolt_gen(&g507, &b507[s], 0.0f, 0.0f, cosf(a), sinf(a), 0.13f, 6, 6, 0.65f);
            hue507[s] = gk_rf(&g507);
            bi507[s] = idx;
        }
        float env = gk_env(age, 20.0f, 300.0f, 140.0f);
        if (env <= 0.0f) continue;
        float prog = age / 300.0f;
        float breathe = 0.88f + 0.12f * sinf(age * 0.02f);
        /* hue runs centre -> tips, steps per fork, drifts slowly with age */
        int pi = base + (int)(hue507[s] * 7000.0f) + (int)(age * 4.0f);
        hk_style st;
        hk_style_set(&st, 5000, 1800, 800,
                     0.4f * env * breathe, 1.7f * sc, 5.5f * sc, 0.5f,
                     0.40f, 0.6f * env, 0.75f * sc, 1.9f * sc, 0.25f);
        float rot = t * 0.0006f * (s ? -1.0f : 1.0f);
        hk_kaleido(&g507, &b507[s], cx, cy, rot, rmax, 6, 1, prog, 1.0f, pal, pi, &st);
    }
    float hub[3];
    gk_col(pal, base + 2000, 0.6f, 0.9f, hub);
    gk_dot(&g507, cx, cy, hub, 3.0f * sc, 12.0f * sc, 0.5f);
    gk_present(&g507, fb, w, h);
}
