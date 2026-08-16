/* 481 Plasma Globe — a glass sphere with a bright electrode at its centre
 * and a dozen slow filaments reaching to the inner wall, each one wandering
 * around the rim on its own noise clock and re-jagging so gradually (cross-
 * fading between two channel geometries) that the tendrils appear to swim.
 * The rim glows faintly where filaments touch it.  Figure overlay.
 * Repaint pattern. */
#include "_hue469.h"

#define NF481 12
#define P481 160

static gk g481;
static gk_bolt b481[NF481][2];
static int bi481[NF481][2];
static uint32_t bs481 = 0xFFFFFFFFu;

static void gen481(int f, int j, int idx, uint32_t seed)
{
    gk_seed(&g481, seed ^ (uint32_t)(idx * 6007 + f * 3001 + j * 977));
    /* unit space: from centre to (1,0), curved outward */
    gk_bolt_gen(&g481, &b481[f][j], 0.05f, 0.0f, 1.0f, 0.0f, 0.12f, 6, 2, 0.3f);
    bi481[f][j] = idx;
}

void pattern_481(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g481, w, h);
    gk_clear(&g481);
    if (seed != bs481) { memset(bi481, 0xFF, sizeof bi481); bs481 = seed; }
    float cw = (float)g481.cw, ch = (float)g481.ch, sc = g481.sc, t = (float)frame;
    float cx = cw * 0.5f, cy = ch * 0.5f;
    float R = (cw < ch ? cw : ch) * 0.42f;
    int base = (int)(t * 1.6f) + (int)(seed & 8191u);
    /* glass rim, faint */
    float rim[3];
    gk_col(pal, base + 6000, 0.1f, 0.16f, rim);
    gk_ring(&g481, cx, cy, R, 2.5f * sc, rim);
    for (int f = 0; f < NF481; f++) {
        /* two geometries per filament crossfading: j=0 on the even clock, j=1 on the odd */
        float ang = GK_TAU * (float)f / (float)NF481 + 0.9f * (gk_noise1(t * 0.003f + (float)f * 2.7f, (uint32_t)f + (seed & 511u)) - 0.5f) + t * 0.0008f;
        float len = 0.92f + 0.05f * sinf(t * 0.02f + (float)f);
        int pi = base + f * 700;
        hk_style st;                                  /* hue electrode -> glass */
        hk_style_set(&st, 3500, 1200, 900,
                     0.35f, 1.6f * sc, 5.5f * sc, 0.5f,
                     0.40f, 0.5f, 0.7f * sc, 1.8f * sc, 0.2f);
        for (int j = 0; j < 2; j++) {
            int ph = frame + j * (P481 / 2) + f * 13;
            int idx = ph / P481;
            float age = (float)(ph - idx * P481);
            if (idx != bi481[f][j]) gen481(f, j, idx, seed);
            /* triangle-ish crossfade: up 40, hold 40, down 40 => sum of two ~ 1 */
            float env = gk_env(age, 45.0f, 30.0f, 45.0f);
            if (env <= 0.0f) continue;
            hk_bolt_xf(&g481, &b481[f][j], cx, cy, ang, R * len, 2.0f, env, pal, pi + (int)(age * 4.0f), &st);
        }
        /* rim touch glow */
        float tg[3];
        gk_col(pal, pi, 0.3f, 0.5f, tg);
        gk_dot(&g481, cx + cosf(ang) * R * len, cy + sinf(ang) * R * len, tg, 2.0f * sc, 8.0f * sc, 0.6f);
    }
    float el[3];
    gk_col(pal, base + 2000, 0.6f, 1.6f, el);
    gk_dot(&g481, cx, cy, el, 5.0f * sc, 24.0f * sc, 0.5f);
    gk_present(&g481, fb, w, h);
}
