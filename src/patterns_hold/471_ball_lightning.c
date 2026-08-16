/* 471 Ball Lightning — four luminous globes wandering on noise paths, each
 * wrapped in a slow crackle of short jagged filaments that grow out of the
 * ball, hold, and retract (each on its own long clock, so the crackle is a
 * gentle churn, never a flicker).  Sparse overlay, black elsewhere.
 * Repaint pattern. */
#include "_hue469.h"

#define NB471 4
#define NFIL471 6
#define PFIL471 90

static gk g471;
static gk_bolt fil471[NB471][NFIL471];
static int filidx471[NB471][NFIL471];
static uint32_t filseed471;

/* filaments live in ball-local units (radius 1) and are transformed on draw */
static void genfil(int b, int f, int idx, uint32_t seed)
{
    gk_seed(&g471, seed ^ (uint32_t)(idx * 7907 + b * 4111 + f * 611));
    float ang = gk_rf(&g471) * GK_TAU;
    float len = 1.4f + 1.6f * gk_rf(&g471);
    gk_bolt_gen(&g471, &fil471[b][f], cosf(ang) * 0.5f, sinf(ang) * 0.5f,
                cosf(ang) * len, sinf(ang) * len, 0.28f, 4, 2, 0.5f);
    filidx471[b][f] = idx;
}

void pattern_471(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g471, w, h);
    gk_clear(&g471);
    float cw = (float)g471.cw, ch = (float)g471.ch, sc = g471.sc;
    float t = (float)frame;
    if (seed != filseed471) { memset(filidx471, 0xFF, sizeof filidx471); filseed471 = seed; }
    int base = (int)(t * 2.0f) + (int)(seed & 8191u);
    for (int b = 0; b < NB471; b++) {
        uint32_t k = (uint32_t)b * 31u + (seed & 1023u);
        float bx = cw * (0.12f + 0.76f * gk_noise1(t * 0.0021f + (float)b * 5.1f, k));
        float by = ch * (0.12f + 0.76f * gk_noise1(t * 0.0017f + (float)b * 9.7f, k + 7u));
        float rad = sc * (11.0f + 7.0f * gk_noise1(t * 0.004f, k + 3u));
        int pi = base + b * 2600;
        float col[3], hal[3];
        gk_col(pal, pi, 0.6f, 1.8f, col);
        gk_col(pal, pi + 700, 0.0f, 0.55f, hal);
        /* the ball: soft body, wide halo, hot core */
        gk_disc(&g471, bx, by, rad * 3.2f, hal);
        gk_dot(&g471, bx, by, col, rad * 0.55f, rad * 1.5f, 0.6f);
        /* filaments */
        for (int f = 0; f < NFIL471; f++) {
            int ph = frame + f * (PFIL471 / NFIL471) + b * 13;
            int idx = ph / PFIL471;
            float age = (float)(ph - idx * PFIL471);
            if (idx != filidx471[b][f]) genfil(b, f, idx, seed);
            float env = gk_env(age, 20.0f, 25.0f, 35.0f);
            if (env <= 0.0f) continue;
            /* filament hue drifts outward from the ball's colour and morphs with age */
            hk_style st;
            hk_style_set(&st, 2500, 900, 400,
                         0.25f, 1.6f * sc, 4.5f * sc, 0.4f,
                         0.35f, 0.8f, 0.9f * sc, 3.0f * sc, 0.3f);
            hk_bolt_xf(&g471, &fil471[b][f], bx, by, t * 0.003f + (float)f, rad,
                       age / 22.0f, env, pal, pi + 300 + (int)(age * 10.0f), &st);
        }
    }
    gk_present(&g471, fb, w, h);
}
