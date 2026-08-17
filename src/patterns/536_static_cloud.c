/* 536 Static Cloud — a fuzzy sphere of charge drifting slowly about the
 * frame: inside a soft luminous ball, forty tiny arcs flicker into being
 * between drifting points, each one growing over ~12 frames, holding and
 * fading over ~30 on its own clock (staggered, so the crackle is a gentle
 * churn); arcs are coloured by their position in the ball (a hue gradient
 * across it) plus a per-arc shift, and the ball's own hue drifts.  Sparse
 * overlay, black beyond the ball.  Repaint. */
#include "_trace509.h"

#define NA536 40

static gk g536;
static gk_bolt b536[NA536];
static int bi536[NA536];
static uint32_t bs536 = 0xFFFFFFFFu;
static float ax536[NA536], ay536[NA536], bx536[NA536], by536[NA536];

void pattern_536(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g536, w, h);
    gk_clear(&g536);
    if (seed != bs536) { for (int a = 0; a < NA536; a++) bi536[a] = -1; bs536 = seed; }
    float cw = (float)g536.cw, ch = (float)g536.ch, sc = g536.sc, t = (float)frame;
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    float R = (cw < ch ? cw : ch) * 0.24f;
    float cx = cw * (0.5f + 0.25f * (gk_noise1(t * 0.0018f, 3u + seed) - 0.5f) * 2.0f);
    float cy = ch * (0.5f + 0.22f * (gk_noise1(t * 0.0015f + 9.0f, 4u + seed) - 0.5f) * 2.0f);
    /* the ball */
    float k0[3], k1[3], k2[3];
    gk_col(pal, base, 0.3f, 0.30f, k0);
    gk_col(pal, base + 1200, 0.1f, 0.14f, k1);
    gk_col(pal, base + 2400, 0.05f, 0.06f, k2);
    gk_dot(&g536, cx, cy, k2, R * 0.9f, R * 1.3f, 0.5f);
    gk_dot(&g536, cx, cy, k1, R * 0.5f, R * 0.9f, 0.6f);
    gk_dot(&g536, cx, cy, k0, R * 0.15f, R * 0.4f, 0.6f);
    for (int a = 0; a < NA536; a++) {
        int P = 55 + (int)(gk_hash((uint32_t)a * 131u + seed) * 50.0f);
        int ph = frame + (int)(gk_hash((uint32_t)a * 171u + seed) * (float)P);
        int idx = ph / P;
        float age = (float)(ph - idx * P);
        if (bi536[a] != idx) {
            gk_seed(&g536, seed ^ (uint32_t)(idx * 2381 + a * 907));
            float r1 = sqrtf(gk_rf(&g536)), a1 = gk_rf(&g536) * GK_TAU;
            ax536[a] = cosf(a1) * r1; ay536[a] = sinf(a1) * r1;
            float a2 = a1 + (0.6f + 1.2f * gk_rf(&g536)) * (gk_rf(&g536) < 0.5f ? -1.0f : 1.0f);
            float r2 = 0.3f + 0.7f * gk_rf(&g536);
            bx536[a] = cosf(a2) * r2; by536[a] = sinf(a2) * r2;
            gk_bolt_gen(&g536, &b536[a], 0.0f, 0.0f, 1.0f, 0.0f, 0.18f, 4, 1, 0.3f);
            bi536[a] = idx;
        }
        float env = gk_env(age, 12.0f, 10.0f, 30.0f);
        if (env <= 0.0f) continue;
        float x0 = cx + ax536[a] * R, y0 = cy + ay536[a] * R;
        float x1 = cx + bx536[a] * R, y1 = cy + by536[a] * R;
        float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
        int pi = base + (int)((ax536[a] + 1.0f) * 1500.0f) + (int)(gk_hash((uint32_t)idx * 7u + (uint32_t)a) * 1200.0f);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.35f * env, h0);
        gk_col(pal, pi + 900, 0.05f, 0.35f * env, h1);
        gk_col(pal, pi + 300, 0.5f, 0.6f * env, c0);
        gk_col(pal, pi + 1200, 0.5f, 0.6f * env, c1);
        bx_draw_grad(&g536, &b536[a], x0, y0, ang, len, 1.0f, age / 12.0f, 1.0f, h0, h1, 0.0f, 1.3f * sc, 4.5f * sc, 0.5f);
        bx_draw_grad(&g536, &b536[a], x0, y0, ang, len, 1.0f, age / 12.0f, 1.0f, c0, c1, 0.0f, 0.6f * sc, 1.5f * sc, 0.25f);
    }
    gk_present(&g536, fb, w, h);
}
