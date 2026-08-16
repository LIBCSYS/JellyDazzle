/* 473 Chain Lightning — seven glowing nodes drift about the frame; a
 * discharge hops from node to node in sequence, each hop a jagged arc that
 * reaches across in ~24 frames, holds, and fades while the next hop is
 * already leaving.  Nodes brighten as the charge arrives.  Sparse overlay.
 * Repaint pattern. */
#include "_hue469.h"

#define NN473 7
#define PH473 44          /* frames per hop */

static gk g473;
static gk_bolt arc473[3];
static int ai473[3] = { -1, -1, -1 };
static uint32_t as473;

static int node473(int hi, uint32_t seed)
{
    if (hi < 0) hi = 0;
    int r = (int)(gk_hash((uint32_t)hi * 77u + seed) * NN473) % NN473;
    int q = (int)(gk_hash((uint32_t)(hi - 1) * 77u + seed) * NN473) % NN473;
    return r == q ? (r + 1) % NN473 : r;
}

static void nodepos(int i, float t, uint32_t seed, float cw, float ch, float *x, float *y)
{
    uint32_t k = (uint32_t)i * 17u + (seed & 4095u);
    *x = cw * (0.1f + 0.8f * gk_noise1(t * 0.0013f + (float)i * 3.3f, k));
    *y = ch * (0.1f + 0.8f * gk_noise1(t * 0.0011f + (float)i * 7.1f, k + 5u));
}

void pattern_473(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g473, w, h);
    gk_clear(&g473);
    if (seed != as473) { ai473[0] = ai473[1] = ai473[2] = -1; as473 = seed; }
    float cw = (float)g473.cw, ch = (float)g473.ch, sc = g473.sc, t = (float)frame;
    int base = (int)(t * 2.1f) + (int)(seed & 8191u);
    float nx[NN473], ny[NN473], glow[NN473];
    for (int i = 0; i < NN473; i++) { nodepos(i, t, seed, cw, ch, &nx[i], &ny[i]); glow[i] = 0.0f; }
    int hop = frame / PH473;
    /* three hops alive at once: current, previous, one before */
    for (int j = 0; j < 3; j++) {
        int hi = hop - j;
        if (hi < 0) continue;
        float age = (float)(frame - hi * PH473);
        float env = gk_env(age, 6.0f, 30.0f, 60.0f);
        if (env <= 0.0f) continue;
        /* hop hi goes from node(hi-1) to node(hi) */
        int a = node473(hi - 1, seed), b = node473(hi, seed);
        int slot = hi % 3;
        if (ai473[slot] != hi) {
            /* generate in unit space from (0,0) to (1,0); stretched on draw */
            gk_seed(&g473, seed ^ (uint32_t)(hi * 2971));
            gk_bolt_gen(&g473, &arc473[slot], 0.0f, 0.0f, 1.0f, 0.0f, 0.13f, 6, 3, 0.35f);
            ai473[slot] = hi;
        }
        float dx = nx[b] - nx[a], dy = ny[b] - ny[a];
        float len = sqrtf(dx * dx + dy * dy), rot = atan2f(dy, dx);
        float prog = age / 24.0f;
        /* each hop starts in its own hue and grades toward the next node's
         * colour along the arc; drifts while it burns */
        int pi = base + hi * 900 + (int)(age * 12.0f);
        hk_style st;
        hk_style_set(&st, 4500, 1600, 800,
                     0.7f * env, 2.2f * sc, 7.0f * sc, 0.5f,
                     0.40f, 1.3f * env, 1.0f * sc, 2.4f * sc, 0.25f);
        hk_bolt_xf(&g473, &arc473[slot], nx[a], ny[a], rot, len, prog, 1.0f, pal, pi, &st);
        glow[a] += env * 0.6f;
        if (prog >= 1.0f) glow[b] += env * gk_smooth((prog - 1.0f) * 3.0f);
    }
    for (int i = 0; i < NN473; i++) {
        float c[3];
        gk_col(pal, base + i * 1500, 0.3f, 1.0f + 1.5f * glow[i], c);
        gk_dot(&g473, nx[i], ny[i], c, 2.5f * sc, (9.0f + 5.0f * glow[i]) * sc, 0.6f);
    }
    gk_present(&g473, fb, w, h);
}
