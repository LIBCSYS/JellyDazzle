/* 548 Soap Bubbles — thin-walled spheres drift up and sideways, wobbling;
 * the rim is iridescent (hue runs round the circumference and slides with
 * time), a crescent highlight sits high-left, and a bubble is born small
 * and quietly fades rather than bursting.  Figure overlay, repaint. */
#include "_fig541.h"

#define NB548 10
#define P548 760.0f
static gk g548;

void pattern_548(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g548, w, h);
    gk_clear(&g548);
    float cw = (float)g548.cw, ch = (float)g548.ch, sc = g548.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NB548; i++) {
        float ph = t + gk_hash(seed + (uint32_t)i * 17u) * P548;
        int cyc = (int)floorf(ph / P548);
        float age = ph - (float)cyc * P548;
        uint32_t s = seed + (uint32_t)i * 311u + (uint32_t)cyc * 6151u;
        float life = fg_life(age, P548, 110.0f) * amp;
        if (life <= 0.0f) continue;
        float u = age / P548;
        float R = (14.0f + 34.0f * gk_hash(s + 1u)) * sc * (0.6f + 0.4f * gk_smooth(age / 150.0f));
        float x = cw * (0.1f + 0.8f * gk_hash(s + 2u)) + cw * 0.06f * sinf(t * 0.005f + gk_hash(s + 3u) * 6.0f);
        float y = ch * (1.08f - 1.2f * u) + ch * 0.02f * sinf(t * 0.011f + (float)i);
        float wob = 0.06f * sinf(t * 0.017f + gk_hash(s + 4u) * 6.0f);
        float hb = fg_pick_sat(pal, gk_hash(s + 5u) * 32768.0f, 6000.0f);
        float c[3];
        int segs = 40;
        float lx = 0.0f, ly = 0.0f;
        for (k = 0; k <= segs; k++) {
            float a = GK_TAU * (float)k / (float)segs;
            float rr = R * (1.0f + wob * cosf(2.0f * a + t * 0.02f));
            float px = x + cosf(a) * rr, py = y + sinf(a) * rr * (1.0f - wob);
            if (k) {
                float idx = hb + 3000.0f * (0.5f + 0.5f * sinf(a * 2.0f + t * 0.012f)) + 1500.0f * sinf(a * 3.0f - t * 0.007f);
                fg_colv(pal, idx, 1.4f, life * 0.55f, c);
                gk_seg(&g548, lx, ly, px, py, c, 1.1f * sc, 3.5f * sc, 0.4f);
            }
            lx = px; ly = py;
        }
        /* highlight crescent, upper left */
        for (k = 0; k < 8; k++) {
            float a0 = -2.6f + (float)k * 0.14f, a1 = a0 + 0.14f;
            float rr = R * 0.78f;
            fg_colv(pal, hb + 4500.0f + (float)k * 300.0f, 1.1f, life * 0.7f * (1.0f - (float)k / 9.0f), c);
            gk_seg(&g548, x + cosf(a0) * rr, y + sinf(a0) * rr, x + cosf(a1) * rr, y + sinf(a1) * rr, c, 1.4f * sc, 3.5f * sc, 0.4f);
        }
        /* faint fill so the film reads */
        fg_colv(pal, hb + 8000.0f, 1.2f, life * 0.045f, c);
        gk_disc(&g548, x, y, R, c);
    }
    gk_present(&g548, fb, w, h);
}
