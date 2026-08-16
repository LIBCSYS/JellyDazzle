/* 539 Spark Rain — charged rain: two dozen faint drops fall slowly as
 * short streaks through the dark, and where each one meets the ground
 * line it splashes into a little burst of three or four tiny arcs that
 * spread along the ground, bloom over ~10 frames and fade over ~40 with
 * a soft ring of light around the landing.  Every drop carries its own
 * hue; splash arcs shift hue outward; the ground line breathes with the
 * drifting base hue.  Sparse field.  Repaint. */
#include "_trace509.h"

#define ND539 24

static gk g539;
static gk_bolt b539[ND539][4];
static int bi539[ND539];
static uint32_t bs539 = 0xFFFFFFFFu;

void pattern_539(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g539, w, h);
    gk_clear(&g539);
    if (seed != bs539) { for (int d = 0; d < ND539; d++) bi539[d] = -1; bs539 = seed; }
    float cw = (float)g539.cw, ch = (float)g539.ch, sc = g539.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float gy = ch * 0.82f;
    float gc[3];
    gk_col(pal, base + 4000, 0.15f, 0.08f + 0.02f * sinf(t * 0.01f), gc);
    gk_seg(&g539, 0.0f, gy, cw, gy, gc, 1.0f * sc, 3.5f * sc, 0.4f);
    for (int d = 0; d < ND539; d++) {
        int P = 200 + (int)(gk_hash((uint32_t)d * 131u + seed) * 120.0f);
        int ph = frame + (int)(gk_hash((uint32_t)d * 173u + seed) * (float)P);
        int idx = ph / P;
        float age = (float)(ph - idx * P);
        uint32_t hs = (uint32_t)idx * 4787u + (uint32_t)d * 5779u + seed;
        float x = cw * (0.03f + 0.94f * gk_hash(hs + 1u));
        float fallT = 110.0f + 40.0f * gk_hash(hs + 2u);
        int pi = base + (int)(gk_hash(hs + 3u) * 8000.0f);
        if (age < fallT) {
            /* falling streak, easing in from the top */
            float u = age / fallT;
            float y = -ch * 0.05f + (gy + ch * 0.05f) * u;
            float lenp = 14.0f * sc;
            float ain = gk_smooth(u / 0.15f) * (1.0f - gk_smooth((u - 0.88f) / 0.12f));
            float c[3], hc[3];
            gk_col(pal, pi, 0.3f, 0.5f * ain, c);
            gk_col(pal, pi + 600, 0.05f, 0.25f * ain, hc);
            gk_seg(&g539, x, y - lenp, x, y, hc, 1.4f * sc, 4.0f * sc, 0.5f);
            gk_seg(&g539, x, y - lenp, x, y, c, 0.7f * sc, 1.6f * sc, 0.25f);
        } else {
            float sa = age - fallT;
            if (bi539[d] != idx) {
                gk_seed(&g539, seed ^ (uint32_t)(idx * 2333 + d * 761));
                for (int k = 0; k < 4; k++) gk_bolt_gen(&g539, &b539[d][k], 0.0f, 0.0f, 1.0f, 0.0f, 0.22f, 4, 1, 0.3f);
                bi539[d] = idx;
            }
            float env = gk_env(sa, 10.0f, 12.0f, 40.0f);
            if (env <= 0.0f) continue;
            int nk = 3 + ((int)(gk_hash(hs + 4u) * 2.0f) & 1);
            for (int k = 0; k < nk; k++) {
                float dir = (k & 1) ? -1.0f : 1.0f;
                float el = 0.15f + 1.1f * gk_hash(hs + 10u + (uint32_t)k);           /* elevation above the ground */
                float ax = dir * cosf(el), ay = -sinf(el) * 0.6f;
                float len = (22.0f + 40.0f * gk_hash(hs + 20u + (uint32_t)k)) * sc;
                float c0[3], c1[3], h0[3], h1[3];
                gk_col(pal, pi, 0.05f, 0.35f * env, h0);
                gk_col(pal, pi + 1400, 0.05f, 0.25f * env, h1);
                gk_col(pal, pi + 300, 0.5f, 0.6f * env, c0);
                gk_col(pal, pi + 1700, 0.35f, 0.45f * env, c1);
                float a2 = atan2f(ay, ax);
                bx_draw_grad(&g539, &b539[d][k], x, gy, a2, len, 1.0f, sa / 10.0f, 1.0f, h0, h1, 0.4f, 1.3f * sc, 4.5f * sc, 0.5f);
                bx_draw_grad(&g539, &b539[d][k], x, gy, a2, len, 1.0f, sa / 10.0f, 1.0f, c0, c1, 0.4f, 0.6f * sc, 1.5f * sc, 0.25f);
            }
            float rc[3];
            gk_col(pal, pi + 800, 0.2f, 0.4f * env, rc);
            gk_dot(&g539, x, gy, rc, 4.0f * sc, 16.0f * sc, 0.6f);
        }
    }
    gk_present(&g539, fb, w, h);
}
