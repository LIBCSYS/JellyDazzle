/* 545 Spinning Stars — six outline stars (4 to 8 points) turning at their
 * own slow rates while drifting; each point takes its own morphing palette
 * offset, and a soft filled star half the size glows inside, so the figure
 * reads as an object.  Figure overlay, repaint. */
#include "_fig541.h"

#define NS545 6
static gk g545;

void pattern_545(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g545, w, h);
    gk_clear(&g545);
    float cw = (float)g545.cw, ch = (float)g545.ch, sc = g545.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NS545; i++) {
        uint32_t s = seed + (uint32_t)i * 977u;
        int n = 4 + (int)(gk_hash(s + 1u) * 5.0f);
        float R = (30.0f + 40.0f * gk_hash(s + 2u)) * sc, r = R * (0.38f + 0.14f * gk_hash(s + 3u));
        float px = cw * (0.5f + 0.40f * sinf(t * (0.0009f + 0.0008f * gk_hash(s + 4u)) + gk_hash(s + 5u) * GK_TAU));
        float py = ch * (0.5f + 0.38f * sinf(t * (0.0007f + 0.0009f * gk_hash(s + 6u)) + gk_hash(s + 7u) * GK_TAU));
        float rot = t * (0.003f + 0.006f * gk_hash(s + 8u)) * (gk_hash(s + 9u) < 0.5f ? -1.0f : 1.0f);
        float hb = fg_pick_sat(pal, gk_hash(s + 10u) * 32768.0f, 6000.0f) + 1000.0f * sinf(t * 0.004f + (float)i);
        float c[3];
        float xs[18], ys[18];
        for (k = 0; k < 2 * n; k++) {
            float a = rot + GK_TAU * (float)k / (float)(2 * n);
            float rr = (k & 1) ? r : R;
            xs[k] = px + cosf(a) * rr; ys[k] = py + sinf(a) * rr;
        }
        for (k = 0; k < 2 * n; k++) {
            int j = (k + 1) % (2 * n);
            fg_colv(pal, hb + (float)(k >> 1) * (2600.0f / (float)n) + 700.0f * sinf(t * 0.007f + (float)k), 1.3f, amp * 0.9f, c);
            gk_seg(&g545, xs[k], ys[k], xs[j], ys[j], c, 1.5f * sc, 4.5f * sc, 0.35f);
        }
        /* inner soft fill: fan of triangles from centre, dim */
        for (k = 0; k < 2 * n; k++) {
            int j = (k + 1) % (2 * n);
            fg_colv(pal, hb + 3500.0f + (float)(k >> 1) * (2000.0f / (float)n), 1.2f, amp * 0.22f, c);
            fg_tri(&g545, px, py, px + (xs[k] - px) * 0.55f, py + (ys[k] - py) * 0.55f,
                   px + (xs[j] - px) * 0.55f, py + (ys[j] - py) * 0.55f, c);
        }
        fg_colv(pal, hb + 5000.0f, 1.2f, amp * 0.6f, c);
        gk_dot(&g545, px, py, c, 2.0f * sc, 8.0f * sc, 0.3f);
    }
    gk_present(&g545, fb, w, h);
}
