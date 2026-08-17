/* 547 Slow Meteors — meteors cross the frame at a stately pace on shallow
 * diagonals, each with a long tapering tail whose hue morphs from head to
 * tip; a head arrives dim, brightens, and dies away before it leaves.
 * Several in flight at once, staggered.  Figure overlay, repaint. */
#include "_fig541.h"

#define NM547 5
#define P547 520.0f
static gk g547;

void pattern_547(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g547, w, h);
    gk_clear(&g547);
    float cw = (float)g547.cw, ch = (float)g547.ch, sc = g547.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NM547; i++) {
        float ph = t + (float)i * (P547 / (float)NM547);
        int cyc = (int)floorf(ph / P547);
        float age = ph - (float)cyc * P547;
        uint32_t s = seed + (uint32_t)i * 233u + (uint32_t)cyc * 4099u;
        float life = fg_life(age, P547, 120.0f) * amp;
        if (life <= 0.0f) continue;
        float dir = gk_hash(s + 1u) < 0.5f ? 1.0f : -1.0f;
        float ang = (0.25f + 0.35f * gk_hash(s + 2u));            /* below horizontal */
        float x0 = dir > 0.0f ? -cw * 0.1f : cw * 1.1f;
        float y0 = ch * (0.05f + 0.55f * gk_hash(s + 3u));
        float len = cw * 1.25f;
        float u = age / P547;
        float dx = cosf(ang) * dir, dy = sinf(ang);
        float px = x0 + dx * len * u, py = y0 + dy * len * u;
        float hb = fg_pick_sat(pal, gk_hash(s + 4u) * 32768.0f, 6000.0f) + 1200.0f * sinf(t * 0.005f + (float)i);
        float tl = (90.0f + 70.0f * gk_hash(s + 5u)) * sc;
        float c[3];
        int segs = 18;
        for (k = 0; k < segs; k++) {
            float a = (float)k / (float)segs, b = (float)(k + 1) / (float)segs;
            float f = (1.0f - a); f = f * f;
            fg_colv(pal, hb + a * 4500.0f, 1.3f, life * 0.8f * f, c);
            float wd = (3.2f - 2.6f * a) * sc;
            gk_seg(&g547, px - dx * tl * a, py - dy * tl * a, px - dx * tl * b, py - dy * tl * b, c, wd, wd * 3.0f + 3.0f * sc, 0.3f);
        }
        fg_colv(pal, hb, 1.2f, life * 0.9f, c);
        gk_dot(&g547, px, py, c, 3.5f * sc, 14.0f * sc, 0.35f);
        gk_col(pal, (int)hb, 0.5f, life * 1.0f, c);
        gk_dot(&g547, px, py, c, 2.0f * sc, 6.0f * sc, 0.3f);
    }
    gk_present(&g547, fb, w, h);
}
