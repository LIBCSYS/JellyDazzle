/* 497 Sprite Lightning — red sprites: the huge, faint, upward discharges
 * that bloom above thunderstorms.  Each sprite is a cluster of tendrils
 * rising from a bright band, fanning up and outward like a carrot-shaped
 * jellyfish, blooming over ~30 frames and dissolving over ~90 in the
 * palette's dim end.  Two sprite slots high in the frame.  Sparse-to-
 * figure overlay.  Repaint pattern. */
#include "_hue469.h"

#define NS497 2
#define NT497 9
#define P497 300

static gk g497;
static gk_bolt tend497[NS497][NT497];
static int bi497[NS497] = { -1, -1 };
static uint32_t bs497 = 0xFFFFFFFFu;
static float sx497[NS497], sy497[NS497], sw497[NS497];

void pattern_497(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g497, w, h);
    gk_clear(&g497);
    if (seed != bs497) { bi497[0] = bi497[1] = -1; bs497 = seed; }
    float cw = (float)g497.cw, ch = (float)g497.ch, sc = g497.sc, t = (float)frame;
    int base = (int)(t * 1.2f) + (int)(seed & 8191u);
    for (int s = 0; s < NS497; s++) {
        int ph = frame + s * (P497 / 2);
        int idx = ph / P497;
        float age = (float)(ph - idx * P497);
        if (idx != bi497[s]) {
            gk_seed(&g497, seed ^ (uint32_t)(idx * 2609 + s * 7331));
            sx497[s] = cw * (0.2f + 0.6f * gk_rf(&g497));
            sy497[s] = ch * (0.55f + 0.2f * gk_rf(&g497));      /* base band height */
            sw497[s] = cw * (0.10f + 0.12f * gk_rf(&g497));
            for (int k = 0; k < NT497; k++) {
                float u = ((float)k + 0.5f) / (float)NT497 - 0.5f;   /* -0.5..0.5 */
                float x0 = sx497[s] + u * sw497[s] * 0.6f;
                float len = ch * (0.18f + 0.22f * gk_rf(&g497)) * (1.0f - 0.8f * u * u);
                float x1 = x0 + u * sw497[s] * 1.6f + cw * 0.03f * gk_rs(&g497);
                gk_bolt_gen(&g497, &tend497[s][k], x0, sy497[s], x1, sy497[s] - len, 0.14f, 5, 2, 0.35f);
            }
            bi497[s] = idx;
        }
        float env = gk_env(age, 30.0f, 40.0f, 90.0f);
        if (env <= 0.0f) continue;
        int pi = base + s * 3000 + (int)(age * 8.0f);
        hk_style st;                      /* hue climbs each tendril; neighbours differ */
        hk_style_set(&st, 3500, 1200, 500,
                     0.30f * env, 2.4f * sc, 9.0f * sc, 0.6f,
                     0.30f, 0.40f * env, 0.9f * sc, 3.0f * sc, 0.3f);
        for (int k = 0; k < NT497; k++) {
            float prog = age / 34.0f - (float)((k * 5) % NT497) * 0.03f;
            hk_bolt(&g497, &tend497[s][k], prog, 1.0f, pal, pi + k * 300, &st);
        }
        float bc[3], hb[3];
        gk_col(pal, pi + 200, 0.5f, 0.6f * env, bc);
        gk_col(pal, pi + 900, 0.0f, 0.18f * env, hb);
        /* the bright band at the base and a diffuse halo above it */
        gk_seg(&g497, sx497[s] - sw497[s] * 0.5f, sy497[s], sx497[s] + sw497[s] * 0.5f, sy497[s], bc, 2.5f * sc, 10.0f * sc, 0.6f);
        gk_disc(&g497, sx497[s], sy497[s] - ch * 0.12f, sw497[s] * 1.4f, hb);
    }
    gk_present(&g497, fb, w, h);
}
