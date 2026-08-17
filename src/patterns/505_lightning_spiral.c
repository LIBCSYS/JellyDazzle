/* 505 Lightning Spiral — a bolt that follows an Archimedean spiral out
 * from the centre, its jag stable per point so the whole thing can turn
 * slowly without shimmer; it grows outward over ~220 frames, holds, fades
 * over ~120, and a fresh spiral (other hand, other hue) starts winding
 * while the last dies.  Figure overlay, centre-weighted.  Repaint. */
#include "_hue469.h"

#define NP505 260
#define P505 480

static gk g505;

static void spiral505(gk *g, const uint32_t *pal, int base, uint32_t seed, int idx, float age,
                      float cx, float cy, float rmax, float rot, float sc)
{
    float env = gk_env(age, 10.0f, 260.0f, 120.0f);
    if (env <= 0.0f) return;
    float prog = age / 220.0f;
    float hand = (idx & 1) ? 1.0f : -1.0f;
    float turns = 3.0f + 1.5f * gk_hash((uint32_t)idx * 11u + seed);
    int pi = base + (int)(gk_hash((uint32_t)idx * 13u + seed) * 6000.0f) + (int)(age * 5.0f);
    hk_style st;                          /* hue winds outward with the spiral */
    hk_style_set(&st, 7000, 0, 700,
                 0.45f * env, 1.8f * sc, 6.0f * sc, 0.5f,
                 0.40f, 0.65f * env, 0.8f * sc, 2.0f * sc, 0.25f);
    float lx = 0.0f, ly = 0.0f;
    for (int i = 0; i < NP505; i++) {
        float u = (float)i / (float)(NP505 - 1);
        if (u > prog) break;
        float r = rmax * (0.03f + 0.97f * u);
        float a = rot + hand * u * turns * GK_TAU;
        float jr = (gk_hash((uint32_t)i * 29u + (uint32_t)idx * 7u + seed) - 0.5f) * 9.0f * sc;
        float ja = (gk_hash((uint32_t)i * 31u + (uint32_t)idx * 5u + seed) - 0.5f) * 0.05f;
        float x = cx + cosf(a + ja) * (r + jr), y = cy + sinf(a + ja) * (r + jr);
        if (i > 0) {
            float fade = 1.0f - 0.35f * u;               /* thinner outward */
            hk_seg(g, lx, ly, x, y, u, 2.0f * fade - 1.0f, fade, pal, pi, &st);
        }
        lx = x; ly = y;
    }
    if (prog < 1.0f) {
        float tc[3];
        gk_col(pal, pi + (int)(prog * 7000.0f), 0.5f, 1.0f * env, tc);
        gk_dot(g, lx, ly, tc, 1.6f * sc, 6.0f * sc, 0.6f);
    }
}

void pattern_505(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g505, w, h);
    gk_clear(&g505);
    float cw = (float)g505.cw, ch = (float)g505.ch, sc = g505.sc, t = (float)frame;
    float cx = cw * 0.5f, cy = ch * 0.5f, rmax = (cw < ch ? cw : ch) * 0.46f;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    for (int s = 0; s < 2; s++) {
        int ph = frame + s * (P505 / 2);
        int idx = ph / P505;
        float age = (float)(ph - idx * P505);
        spiral505(&g505, pal, base, seed, idx * 2 + s, age, cx, cy, rmax, t * 0.002f * (s ? -1.0f : 1.0f), sc);
    }
    float hub[3];
    gk_col(pal, base + 3000, 0.5f, 1.0f, hub);
    gk_dot(&g505, cx, cy, hub, 3.0f * sc, 14.0f * sc, 0.5f);
    gk_present(&g505, fb, w, h);
}
