/* 541 Comet Trio — three comets on slow looping paths, each trailing a long
 * soft tail whose hue morphs from head to tip and drifts over the comet's
 * life.  Heads glow warm-white-of-their-hue; tails thin and cool.  Figure
 * overlay over black, repaint. */
#include "_fig541.h"

#define NC541 3
#define TL541 28

static gk g541;

static void path541(float t, int i, uint32_t seed, float cw, float ch, float *x, float *y)
{
    float a = 0.0019f + 0.0006f * gk_hash(seed + (uint32_t)i * 7u);
    float b = 0.0013f + 0.0007f * gk_hash(seed + (uint32_t)i * 7u + 1u);
    float pa = gk_hash(seed + (uint32_t)i * 7u + 2u) * GK_TAU;
    float pb = gk_hash(seed + (uint32_t)i * 7u + 3u) * GK_TAU;
    *x = cw * (0.5f + 0.40f * sinf(t * a + pa) + 0.06f * sinf(t * a * 2.7f + pb));
    *y = ch * (0.5f + 0.38f * sinf(t * b + pb) + 0.06f * cosf(t * b * 2.3f + pa));
}

void pattern_541(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g541, w, h);
    gk_clear(&g541);
    float cw = (float)g541.cw, ch = (float)g541.ch, sc = g541.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NC541; i++) {
        float hb = fg_pick_sat(pal, gk_hash(seed + 91u + (uint32_t)i) * 32768.0f, 5000.0f);
        float drift = 2500.0f * sinf(t * 0.004f + (float)i * 2.1f);
        float px, py, lx, ly;
        path541(t, i, seed, cw, ch, &px, &py);
        lx = px; ly = py;
        float c[3];
        /* tail: sample the path backwards in time */
        for (k = 1; k <= TL541; k++) {
            float u = (float)k / (float)TL541;
            float x, y;
            path541(t - (float)k * 9.0f, i, seed, cw, ch, &x, &y);
            float fade = (1.0f - u); fade *= fade;
            fg_colv(pal, hb + drift + u * 5000.0f, 1.3f, amp * fade * 0.85f, c);
            float wd = (4.5f - 3.5f * u) * sc;
            gk_seg(&g541, lx, ly, x, y, c, wd, wd * 3.0f + 4.0f * sc, 0.28f);
            lx = x; ly = y;
        }
        /* head */
        fg_colv(pal, hb + drift, 1.2f, amp * 0.9f, c);
        gk_dot(&g541, px, py, c, 5.0f * sc, 16.0f * sc, 0.3f);
        gk_col(pal, (int)(hb + drift), 0.55f, amp * 1.1f, c);
        gk_dot(&g541, px, py, c, 3.0f * sc, 8.0f * sc, 0.3f);
    }
    gk_present(&g541, fb, w, h);
}
