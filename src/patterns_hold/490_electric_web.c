/* 490 Electric Web — sixteen charged motes drift on noise paths; whenever
 * two come within reach a thin jagged arc forms between them, brightening
 * smoothly with proximity and vanishing as they part.  Each pair's arc has
 * its own geometry that re-jags on a slow crossfade so the web shimmers
 * rather than flickers.  Sparse overlay.  Repaint pattern. */
#include "_hue469.h"

#define NM490 16
#define NP490 (NM490 * (NM490 - 1) / 2)
#define P490 100

static gk g490;
static gk_bolt arc490[NP490][2];
static int ai490[NP490][2];
static uint32_t as490 = 0xFFFFFFFFu;

void pattern_490(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g490, w, h);
    gk_clear(&g490);
    if (seed != as490) { memset(ai490, 0xFF, sizeof ai490); as490 = seed; }
    float cw = (float)g490.cw, ch = (float)g490.ch, sc = g490.sc, t = (float)frame;
    float reach = (cw < ch ? cw : ch) * 0.30f;
    int base = (int)(t * 1.8f) + (int)(seed & 8191u);
    float mx[NM490], my[NM490], mg[NM490];
    for (int i = 0; i < NM490; i++) {
        uint32_t k = (uint32_t)i * 13u + (seed & 4095u);
        mx[i] = cw * (0.05f + 0.9f * gk_noise1(t * 0.0016f + (float)i * 4.4f, k));
        my[i] = ch * (0.05f + 0.9f * gk_noise1(t * 0.0013f + (float)i * 8.2f, k + 9u));
        mg[i] = 0.0f;
    }
    int p = 0;
    for (int i = 0; i < NM490; i++)
        for (int j = i + 1; j < NM490; j++, p++) {
            float dx = mx[j] - mx[i], dy = my[j] - my[i];
            float d = sqrtf(dx * dx + dy * dy);
            if (d >= reach) continue;
            float str = gk_smooth((reach - d) / reach * 1.6f);
            if (str <= 0.01f) continue;
            float ang = atan2f(dy, dx);
            /* strand hue grades from mote i's colour toward mote j's */
            int pi = base + i * 500;
            hk_style st;
            hk_style_set(&st, (j - i) * 500, 1200, 700,
                         0.35f * str, 1.4f * sc, 4.5f * sc, 0.5f,
                         0.40f, 0.45f * str, 0.6f * sc, 1.5f * sc, 0.2f);
            for (int q = 0; q < 2; q++) {
                int ph = frame + q * (P490 / 2) + p * 7;
                int idx = ph / P490;
                float age = (float)(ph - idx * P490);
                if (ai490[p][q] != idx) {
                    gk_seed(&g490, seed ^ (uint32_t)(idx * 3169 + p * 421 + q * 89));
                    gk_bolt_gen(&g490, &arc490[p][q], 0.0f, 0.0f, 1.0f, 0.0f, 0.10f, 4, 1, 0.3f);
                    ai490[p][q] = idx;
                }
                float env = gk_env(age, 30.0f, 20.0f, 30.0f) * 1.2f;
                if (env <= 0.0f) continue;
                hk_bolt_xf(&g490, &arc490[p][q], mx[i], my[i], ang, d, 2.0f, env, pal, pi + (int)(age * 6.0f), &st);
            }
            mg[i] += str * 0.4f; mg[j] += str * 0.4f;
        }
    for (int i = 0; i < NM490; i++) {
        float c[3];
        gk_col(pal, base + i * 500, 0.35f, 0.5f + 0.8f * mg[i], c);
        gk_dot(&g490, mx[i], my[i], c, 1.8f * sc, (5.0f + 3.0f * mg[i]) * sc, 0.5f);
    }
    gk_present(&g490, fb, w, h);
}
