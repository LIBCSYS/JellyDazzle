/* 543 Drifting Polygons — seven outline polygons (triangle to octagon) drift
 * on slow looping paths and rotate at their own gentle rates; every edge
 * carries a different palette offset that morphs, and a faint inner shape
 * echoes each one.  Homage to DAZZLE's wireframe polygons.  Figure overlay. */
#include "_fig541.h"

#define NP543 7
static gk g543;

void pattern_543(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g543, w, h);
    gk_clear(&g543);
    float cw = (float)g543.cw, ch = (float)g543.ch, sc = g543.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NP543; i++) {
        uint32_t s = seed + (uint32_t)i * 131u;
        int n = 3 + (i % 6);
        float R = (28.0f + 40.0f * gk_hash(s + 1u)) * sc;
        float ax = 0.0011f + 0.0009f * gk_hash(s + 2u), ay = 0.0009f + 0.0009f * gk_hash(s + 3u);
        float px = cw * (0.5f + 0.40f * sinf(t * ax + gk_hash(s + 4u) * GK_TAU));
        float py = ch * (0.5f + 0.38f * sinf(t * ay + gk_hash(s + 5u) * GK_TAU));
        float rot = t * (0.002f + 0.004f * gk_hash(s + 6u)) * (gk_hash(s + 7u) < 0.5f ? -1.0f : 1.0f);
        float hb = fg_pick_sat(pal, gk_hash(s + 8u) * 32768.0f, 6000.0f) + 1200.0f * sinf(t * 0.0035f + (float)i);
        float c[3];
        float xs[9], ys[9];
        for (k = 0; k < n; k++) {
            float a = rot + GK_TAU * (float)k / (float)n;
            xs[k] = px + cosf(a) * R; ys[k] = py + sinf(a) * R;
        }
        for (k = 0; k < n; k++) {
            int j = (k + 1) % n;
            fg_colv(pal, hb + (float)k * (3000.0f / (float)n) + 900.0f * sinf(t * 0.006f + (float)k * 1.7f), 1.3f, amp * 0.9f, c);
            gk_seg(&g543, xs[k], ys[k], xs[j], ys[j], c, 1.6f * sc, 5.0f * sc, 0.35f);
        }
        /* inner echo, counter-rotating, dimmer */
        for (k = 0; k < n; k++) {
            float a = -rot * 0.7f + GK_TAU * ((float)k + 0.5f) / (float)n;
            xs[k] = px + cosf(a) * R * 0.5f; ys[k] = py + sinf(a) * R * 0.5f;
        }
        fg_colv(pal, hb + 4000.0f, 1.2f, amp * 0.4f, c);
        gk_poly(&g543, xs, ys, n, c, 0.9f * sc, 3.0f * sc, 0.3f);
        /* vertex sparks */
        for (k = 0; k < n; k++) {
            float a = rot + GK_TAU * (float)k / (float)n;
            fg_colv(pal, hb + (float)k * (3000.0f / (float)n) + 1500.0f, 1.2f, amp * 0.6f, c);
            gk_dot(&g543, px + cosf(a) * R, py + sinf(a) * R, c, 1.8f * sc, 6.0f * sc, 0.3f);
        }
    }
    gk_present(&g543, fb, w, h);
}
