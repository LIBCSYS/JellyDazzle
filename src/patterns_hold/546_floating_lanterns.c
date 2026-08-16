/* 546 Floating Lanterns — paper lanterns rise slowly through the frame,
 * swaying, each a soft-edged body with a warm inner glow, a top ring and a
 * hanging tassel.  Their hues differ and drift; a lantern is born dim low
 * in the frame and fades as it leaves the top.  Figure overlay, repaint. */
#include "_fig541.h"

#define NL546 8
#define P546 900.0f
static gk g546;

void pattern_546(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g546, w, h);
    gk_clear(&g546);
    float cw = (float)g546.cw, ch = (float)g546.ch, sc = g546.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i;
    for (i = 0; i < NL546; i++) {
        float off = gk_hash(seed + (uint32_t)i * 13u) * P546;
        float ph = t + off;
        int cyc = (int)floorf(ph / P546);
        float age = ph - (float)cyc * P546;
        uint32_t s = seed + (uint32_t)i * 131u + (uint32_t)cyc * 7919u;
        float life = fg_life(age, P546, 140.0f) * amp;
        if (life <= 0.0f) continue;
        float u = age / P546;
        float x = cw * (0.08f + 0.84f * gk_hash(s + 1u)) + cw * 0.03f * sinf(t * 0.006f + gk_hash(s + 2u) * 6.0f);
        float y = ch * (1.10f - 1.25f * u);
        float sz = (20.0f + 18.0f * gk_hash(s + 3u)) * sc;
        float sway = 0.10f * sinf(t * 0.008f + gk_hash(s + 4u) * 6.0f);
        float hb = fg_pick_sat(pal, gk_hash(s + 5u) * 32768.0f, 6000.0f) + 900.0f * sinf(t * 0.004f + (float)i);
        float ca = cosf(sway), sa = sinf(sway);
        float c[3];
        /* body: rotated ellipse, dim; inner glow bright */
        fg_colv(pal, hb, 1.3f, life * 0.32f, c);
        fg_ellipse(&g546, x, y, sz * 0.72f, sz, sway, c);
        fg_colv(pal, hb + 1500.0f, 1.2f, life * 0.9f, c);
        gk_dot(&g546, x, y + sz * 0.1f, c, sz * 0.35f, sz * 0.9f, 0.5f);
        /* ribs */
        int k;
        fg_colv(pal, hb + 3000.0f, 1.2f, life * 0.35f, c);
        for (k = -1; k <= 1; k++) {
            float ox = (float)k * sz * 0.34f;
            float x0, y0, x1, y1;
            fg_xf(ox, -sz * 0.9f, ca, sa, x, y, &x0, &y0);
            fg_xf(ox, sz * 0.9f, ca, sa, x, y, &x1, &y1);
            gk_seg(&g546, x0, y0, x1, y1, c, 0.6f * sc, 1.5f * sc, 0.2f);
        }
        /* cap and base rings */
        float tx, ty, bx, by;
        fg_xf(0.0f, -sz, ca, sa, x, y, &tx, &ty);
        fg_xf(0.0f, sz, ca, sa, x, y, &bx, &by);
        fg_colv(pal, hb + 4500.0f, 1.2f, life * 0.8f, c);
        gk_seg(&g546, tx - sz * 0.35f * ca, ty - sz * 0.35f * sa, tx + sz * 0.35f * ca, ty + sz * 0.35f * sa, c, 1.2f * sc, 3.0f * sc, 0.3f);
        gk_seg(&g546, bx - sz * 0.3f * ca, by - sz * 0.3f * sa, bx + sz * 0.3f * ca, by + sz * 0.3f * sa, c, 1.2f * sc, 3.0f * sc, 0.3f);
        /* tassel */
        float tex, tey;
        fg_xf(0.0f, sz * 1.6f, ca, sa, x, y, &tex, &tey);
        fg_colv(pal, hb + 6000.0f, 1.2f, life * 0.6f, c);
        gk_seg(&g546, bx, by, tex, tey, c, 0.8f * sc, 2.5f * sc, 0.3f);
        gk_dot(&g546, tex, tey, c, 1.5f * sc, 4.0f * sc, 0.3f);
    }
    gk_present(&g546, fb, w, h);
}
