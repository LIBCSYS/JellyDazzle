/* 538 Lightning Vortex — a slow whirlpool of discharge: a dozen short
 * jagged streaks ride logarithmic spirals inward toward a glowing drain at
 * the centre, each streak a travelling window of arc that fades in at the
 * rim, curls round and in over a few hundred frames, and dies as it
 * reaches the eye, throwing small twigs as it goes.  Streak hue shifts
 * from a rim hue to the drain's hue as it goes in; every streak carries
 * its own offset; all drift with time.  Figure overlay, centre-weighted.
 * Repaint. */
#include "_trace509.h"

#define NS538 12
#define P538 520
#define NP538 22

static gk g538;
static uint32_t bs538 = 0xFFFFFFFFu;

void pattern_538(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g538, w, h);
    gk_clear(&g538);
    bs538 = seed;
    float cw = (float)g538.cw, ch = (float)g538.ch, sc = g538.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float cx = cw * 0.5f + cw * 0.03f * sinf(t * 0.002f), cy = ch * 0.5f + ch * 0.03f * cosf(t * 0.0016f);
    float R = (cw < ch ? cw : ch) * 0.48f;
    float hand = (seed & 1u) ? -1.0f : 1.0f;
    for (int s = 0; s < NS538; s++) {
        int ph = frame + s * (P538 / NS538) + s * 7;
        int idx = ph / P538;
        float age = (float)(ph - idx * P538);
        uint32_t hs = (uint32_t)idx * 2749u + (uint32_t)s * 6577u + seed;
        float u = age / (float)P538;                          /* 0 rim .. 1 eye */
        float env = gk_smooth(u / 0.08f) * (1.0f - gk_smooth((u - 0.88f) / 0.12f));
        if (env <= 0.0f) continue;
        float a0 = gk_hash(hs + 1u) * GK_TAU;
        float turns = 1.0f + 0.5f * gk_hash(hs + 2u);
        int pi = base + (int)(gk_hash(hs + 3u) * 3000.0f);
        float lx = 0.0f, ly = 0.0f;
        for (int i = 0; i < NP538; i++) {
            /* window behind the head: du back along the spiral */
            float ub = u - (float)i * 0.012f;
            if (ub < 0.0f) break;
            float r = R * (1.0f - ub) * (1.0f - ub) * 0.95f + R * 0.03f;
            float a = a0 + hand * ub * turns * GK_TAU;
            float jj = ((gk_hash((uint32_t)i * 29u + hs) - 0.5f) * 22.0f + (gk_noise1(ub * 40.0f + (float)s * 3.0f, hs) - 0.5f) * 8.0f) * sc * (0.3f + 0.7f * (1.0f - ub));
            float x = cx + cosf(a) * (r + jj), y = cy + sinf(a) * (r + jj);
            if (i > 0) {
                float fade = 1.0f - (float)i / (float)NP538; fade *= fade;
                int pj = pi + (int)(ub * 4000.0f);
                float hc[3], c[3];
                gk_col(pal, pj + 700, 0.05f, 0.35f * env * fade, hc);
                gk_col(pal, pj, 0.5f, 0.6f * env * fade, c);
                float th = 0.5f + 0.7f * (1.0f - ub);
                gk_seg(&g538, lx, ly, x, y, hc, 1.8f * sc * th, 6.0f * sc * th, 0.5f);
                gk_seg(&g538, lx, ly, x, y, c, 0.8f * sc * th, 2.0f * sc * th, 0.25f);
                /* twig every few points, outward */
                if ((i % 5) == 2) {
                    float ta = a + (gk_hash((uint32_t)i * 71u + hs) - 0.5f) * 1.2f + (gk_hash((uint32_t)i * 13u + hs) < 0.5f ? 0.0f : GK_TAU * 0.5f);
                    float tl = (10.0f + 16.0f * gk_hash((uint32_t)i * 17u + hs)) * sc * fade;
                    float tc2[3];
                    gk_col(pal, pj + 1500, 0.3f, 0.45f * env * fade, tc2);
                    gk_seg(&g538, x, y, x + cosf(ta) * tl, y + sinf(ta) * tl, tc2, 0.7f * sc, 2.5f * sc, 0.4f);
                }
            } else {
                float tc[3];
                gk_col(pal, pi + 500, 0.55f, 0.8f * env, tc);
                gk_dot(&g538, x, y, tc, 1.6f * sc, 6.0f * sc, 0.6f);
            }
            lx = x; ly = y;
        }
    }
    /* the drain */
    float d0[3], d1[3];
    gk_col(pal, base + 4000, 0.45f, 0.8f, d0);
    gk_col(pal, base + 4600, 0.1f, 0.3f, d1);
    gk_dot(&g538, cx, cy, d1, 8.0f * sc, 30.0f * sc, 0.6f);
    gk_dot(&g538, cx, cy, d0, 3.0f * sc, 10.0f * sc, 0.6f);
    gk_present(&g538, fb, w, h);
}
