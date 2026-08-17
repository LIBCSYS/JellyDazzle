/* 504 Chandelier Bolts — a shallow arc of light hangs across the top of
 * the frame like a rod, and from it a row of short bolts dangle, each one
 * swaying gently on its own pendulum, re-jagging on a slow crossfade, and
 * brightening in a wave that runs along the rod.  Figure overlay across
 * the upper two thirds.  Repaint pattern. */
#include "_hue469.h"

#define NB504 11
#define P504 130

static gk g504;
static gk_bolt b504[NB504][2];
static int bi504[NB504][2];
static uint32_t bs504 = 0xFFFFFFFFu;

void pattern_504(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g504, w, h);
    gk_clear(&g504);
    if (seed != bs504) { memset(bi504, 0xFF, sizeof bi504); bs504 = seed; }
    float cw = (float)g504.cw, ch = (float)g504.ch, sc = g504.sc, t = (float)frame;
    int base = (int)(t * 1.5f) + (int)(seed & 8191u);
    /* the rod: a shallow arc, sagging */
    float rc[3];
    gk_col(pal, base + 4500, 0.3f, 0.5f, rc);
    float ax[NB504], ay[NB504];
    for (int i = 0; i < NB504; i++) {
        float u = ((float)i + 0.5f) / (float)NB504;
        ax[i] = cw * (0.06f + 0.88f * u);
        ay[i] = ch * (0.10f + 0.10f * sinf(u * 3.14159f));
    }
    for (int i = 0; i + 1 < NB504; i++)
        gk_seg(&g504, ax[i], ay[i], ax[i + 1], ay[i + 1], rc, 1.4f * sc, 5.0f * sc, 0.4f);
    for (int i = 0; i < NB504; i++) {
        float u = ((float)i + 0.5f) / (float)NB504;
        float sway = 0.18f * sinf(t * 0.012f + (float)i * 0.9f);
        float wave = 0.5f + 0.5f * sinf(u * 5.0f - t * 0.02f);
        float amp = 0.4f + 0.7f * wave * wave;
        float len = ch * (0.30f + 0.18f * gk_hash((uint32_t)i * 7u + seed));
        int pi = base + i * 450;
        hk_style st;                                  /* hue rail -> drop */
        hk_style_set(&st, 3000, 1000, 700,
                     0.4f * amp, 1.6f * sc, 5.5f * sc, 0.5f,
                     0.40f, 0.6f * amp, 0.7f * sc, 1.9f * sc, 0.25f);
        for (int q = 0; q < 2; q++) {
            int ph = frame + q * (P504 / 2) + i * 23;
            int idx = ph / P504;
            float age = (float)(ph - idx * P504);
            if (bi504[i][q] != idx) {
                gk_seed(&g504, seed ^ (uint32_t)(idx * 2749 + i * 617 + q * 53));
                gk_bolt_gen(&g504, &b504[i][q], 0.0f, 0.0f, 1.0f, 0.0f, 0.14f, 5, 2, 0.35f);
                bi504[i][q] = idx;
            }
            float env = gk_env(age, 38.0f, 26.0f, 38.0f) * 1.15f;
            if (env <= 0.0f) continue;
            hk_bolt_xf(&g504, &b504[i][q], ax[i], ay[i], GK_TAU * 0.25f + sway, len, 2.0f, env, pal, pi + (int)(age * 5.0f), &st);
        }
        float kc[3];
        gk_col(pal, pi, 0.5f, 0.6f + 0.6f * amp, kc);
        gk_dot(&g504, ax[i], ay[i], kc, 2.0f * sc, 6.0f * sc, 0.5f);
        /* a drop of light at the tip */
        gk_col(pal, pi + 3000, 0.4f, 0.5f * amp, kc);
        gk_dot(&g504, ax[i] + cosf(GK_TAU * 0.25f + sway) * len, ay[i] + sinf(GK_TAU * 0.25f + sway) * len, kc, 1.8f * sc, 6.0f * sc, 0.5f);
    }
    gk_present(&g504, fb, w, h);
}
