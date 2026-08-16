/* 552 Koi Shapes — four koi glide on slow looping courses, heading along
 * the path, tails and fins swaying; each body carries a base hue with a
 * patch of a second, and the hues drift.  Soft ripple rings mark where a
 * mouth breaks the surface.  Figure overlay, repaint. */
#include "_fig541.h"

#define NK552 4
static gk g552;

static void path552(float t, uint32_t s, int i, float cw, float ch, float *x, float *y)
{
    float a = 0.0016f + 0.0008f * gk_hash(s + 1u), b = 0.0011f + 0.0009f * gk_hash(s + 2u);
    float pa = gk_hash(s + 3u) * GK_TAU + (float)i * 1.7f, pb = gk_hash(s + 4u) * GK_TAU + (float)i * 2.4f;
    *x = cw * (0.5f + 0.38f * sinf(t * a + pa) + 0.05f * sinf(t * b * 2.2f + pb));
    *y = ch * (0.5f + 0.36f * sinf(t * b + pb) + 0.05f * cosf(t * a * 1.9f + pa));
}

void pattern_552(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g552, w, h);
    gk_clear(&g552);
    float cw = (float)g552.cw, ch = (float)g552.ch, sc = g552.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NK552; i++) {
        uint32_t s = seed + (uint32_t)i * 271u;
        float x, y, xp, yp;
        path552(t, s, i, cw, ch, &x, &y);
        path552(t - 6.0f, s, i, cw, ch, &xp, &yp);
        float hd = atan2f(y - yp, x - xp);
        float ca = cosf(hd), sa = sinf(hd);
        float L = (44.0f + 20.0f * gk_hash(s + 5u)) * sc;
        float sw = sinf(t * 0.045f + (float)i * 1.7f);          /* tail sway */
        float hb = fg_pick_sat(pal, gk_hash(s + 6u) * 32768.0f, 6000.0f) + 800.0f * sinf(t * 0.004f + (float)i);
        float hb2 = fg_pick_sat(pal, gk_hash(s + 7u) * 32768.0f, 6000.0f);
        float c[3];
        /* body: two ellipses (head-heavy) */
        fg_colv(pal, hb, 1.3f, amp * 0.5f, c);
        fg_ellipse(&g552, x, y, L * 0.5f, L * 0.2f, hd, c);
        float hx, hy;
        fg_xf(L * 0.28f, 0.0f, ca, sa, x, y, &hx, &hy);
        fg_ellipse(&g552, hx, hy, L * 0.24f, L * 0.19f, hd, c);
        /* patch */
        float pxx, pyy;
        fg_xf(-L * 0.05f, L * 0.05f, ca, sa, x, y, &pxx, &pyy);
        fg_colv(pal, hb2, 1.3f, amp * 0.5f, c);
        fg_ellipse(&g552, pxx, pyy, L * 0.16f, L * 0.09f, hd + 0.4f, c);
        /* tail: fan of two triangles from tail root, swaying */
        float rx, ry, t1x, t1y, t2x, t2y, tmx, tmy;
        fg_xf(-L * 0.45f, 0.0f, ca, sa, x, y, &rx, &ry);
        fg_xf(-L * 0.95f, -L * 0.28f + sw * L * 0.15f, ca, sa, x, y, &t1x, &t1y);
        fg_xf(-L * 0.95f, L * 0.28f + sw * L * 0.15f, ca, sa, x, y, &t2x, &t2y);
        fg_xf(-L * 0.80f, sw * L * 0.15f, ca, sa, x, y, &tmx, &tmy);
        fg_colv(pal, hb + 2500.0f, 1.3f, amp * 0.42f, c);
        fg_tri(&g552, rx, ry, t1x, t1y, tmx, tmy, c);
        fg_colv(pal, hb + 3500.0f, 1.3f, amp * 0.42f, c);
        fg_tri(&g552, rx, ry, tmx, tmy, t2x, t2y, c);
        /* pectoral fins */
        float f1x, f1y, f2x, f2y, f3x, f3y;
        for (k = -1; k <= 1; k += 2) {
            float fs = (float)k;
            fg_xf(L * 0.15f, fs * L * 0.15f, ca, sa, x, y, &f1x, &f1y);
            fg_xf(-L * 0.10f, fs * (L * 0.34f + sw * L * 0.04f), ca, sa, x, y, &f2x, &f2y);
            fg_xf(-L * 0.12f, fs * L * 0.16f, ca, sa, x, y, &f3x, &f3y);
            fg_colv(pal, hb + 4500.0f, 1.3f, amp * 0.36f, c);
            fg_tri(&g552, f1x, f1y, f2x, f2y, f3x, f3y, c);
        }
        /* eye + mouth ripple */
        float ex, ey;
        fg_xf(L * 0.42f, -L * 0.07f, ca, sa, x, y, &ex, &ey);
        gk_col(pal, (int)(hb + 6000.0f), 0.6f, amp * 0.8f, c);
        gk_dot(&g552, ex, ey, c, 0.9f * sc, 2.0f * sc, 0.2f);
        float rp = fg_fract(t * 0.004f + (float)i * 0.25f);
        float mx, my;
        fg_xf(L * 0.55f, 0.0f, ca, sa, x, y, &mx, &my);
        fg_colv(pal, hb + 8000.0f, 1.2f, amp * 0.35f * (1.0f - rp), c);
        gk_ring(&g552, mx, my, L * (0.2f + 0.9f * rp), 1.5f * sc, c);
    }
    gk_present(&g552, fb, w, h);
}
