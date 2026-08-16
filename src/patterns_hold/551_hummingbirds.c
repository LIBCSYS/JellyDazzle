/* 551 Hummingbirds — abstract birds hover at flower-points and glide
 * between them on eased moves: a teardrop body, needle beak, fanned tail,
 * and two wings that beat in a soft slow blur (an arc pair rather than a
 * strobe).  Iridescent bodies: hue shifts with the body angle and drifts.
 * Figure overlay, repaint. */
#include "_fig541.h"

#define NB551 3
#define HOP551 260.0f
static gk g551;

static void spot551(uint32_t s, int j, float cw, float ch, float *x, float *y)
{
    *x = cw * (0.12f + 0.76f * gk_hash(s + (uint32_t)j * 17u + 1u));
    *y = ch * (0.15f + 0.65f * gk_hash(s + (uint32_t)j * 17u + 2u));
}

void pattern_551(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g551, w, h);
    gk_clear(&g551);
    float cw = (float)g551.cw, ch = (float)g551.ch, sc = g551.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NB551; i++) {
        uint32_t s = seed + (uint32_t)i * 809u;
        float ph = t + gk_hash(s) * HOP551;
        int hop = (int)floorf(ph / HOP551);
        float u = (ph - (float)hop * HOP551) / HOP551;
        /* hover 55%, then glide to the next spot with ease in-out */
        float g = u < 0.55f ? 0.0f : gk_smooth((u - 0.55f) / 0.45f);
        float x0, y0, x1, y1;
        spot551(s, hop, cw, ch, &x0, &y0);
        spot551(s, hop + 1, cw, ch, &x1, &y1);
        float x = x0 + (x1 - x0) * g + cw * 0.006f * sinf(t * 0.05f + (float)i);
        float y = y0 + (y1 - y0) * g + ch * 0.008f * sinf(t * 0.033f + (float)i * 2.0f);
        float face = (x1 >= x0) ? 1.0f : -1.0f;              /* facing direction */
        float pitch = -0.25f + 0.55f * g * (1.0f - g) * 4.0f * ((y1 - y0) / ch);
        float ca = cosf(pitch), sa = sinf(pitch) * face;
        float L = (32.0f + 12.0f * gk_hash(s + 3u)) * sc;
        float hb = fg_pick_sat(pal, gk_hash(s + 4u) * 32768.0f, 6000.0f) + 1000.0f * sinf(t * 0.005f + (float)i);
        float c[3];
        /* body: ellipse angled with pitch */
        fg_colv(pal, hb, 1.4f, amp * 0.6f, c);
        fg_ellipse(&g551, x, y, L * 0.5f, L * 0.28f, pitch * face, c);
        /* head */
        float hx, hy;
        fg_xf(face * L * 0.55f, -L * 0.05f, ca, sa, x, y, &hx, &hy);
        fg_colv(pal, hb + 1800.0f, 1.3f, amp * 0.6f, c);
        fg_ellipse(&g551, hx, hy, L * 0.2f, L * 0.18f, 0.0f, c);
        /* beak */
        float bx, by;
        fg_xf(face * L * 1.15f, 0.0f, ca, sa, x, y, &bx, &by);
        fg_colv(pal, hb + 4000.0f, 1.2f, amp * 0.7f, c);
        gk_seg(&g551, hx, hy, bx, by, c, 0.7f * sc, 1.8f * sc, 0.25f);
        /* tail: two feathers */
        float t1x, t1y, t2x, t2y, txx, tyy;
        fg_xf(-face * L * 0.45f, 0.0f, ca, sa, x, y, &txx, &tyy);
        fg_xf(-face * L * 1.05f, -L * 0.22f, ca, sa, x, y, &t1x, &t1y);
        fg_xf(-face * L * 1.05f, L * 0.22f, ca, sa, x, y, &t2x, &t2y);
        fg_colv(pal, hb + 2600.0f, 1.3f, amp * 0.45f, c);
        fg_tri(&g551, txx, tyy, t1x, t1y, t2x, t2y, c);
        /* wings: slow beat as a blur — several arcs at phases across the stroke */
        float beat = t * 0.09f + (float)i * 2.0f;
        for (k = 0; k < 5; k++) {
            float wa = -0.9f + 1.1f * (0.5f + 0.5f * sinf(beat + (float)k * 0.35f));   /* wing angle up from body */
            float wl = L * 0.95f;
            float wx0, wy0, wx1, wy1, wmx, wmy;
            fg_xf(face * L * 0.05f, -L * 0.1f, ca, sa, x, y, &wx0, &wy0);
            fg_xf(face * L * 0.05f - face * cosf(wa) * wl * 0.4f, -L * 0.1f - sinf(wa + 0.9f) * wl, ca, sa, x, y, &wx1, &wy1);
            fg_xf(face * L * 0.05f - face * cosf(wa) * wl * 0.7f, -L * 0.1f - sinf(wa + 0.9f) * wl * 0.55f, ca, sa, x, y, &wmx, &wmy);
            fg_colv(pal, hb + 5500.0f + (float)k * 400.0f, 1.3f, amp * 0.22f, c);
            fg_tri(&g551, wx0, wy0, wx1, wy1, wmx, wmy, c);
            gk_seg(&g551, wx0, wy0, wx1, wy1, c, 0.6f * sc, 1.5f * sc, 0.2f);
        }
        /* eye */
        gk_col(pal, (int)(hb + 7000.0f), 0.6f, amp * 0.8f, c);
        gk_dot(&g551, hx + face * L * 0.08f, hy - L * 0.04f, c, 0.9f * sc, 2.0f * sc, 0.2f);
        /* flower spot: small glow where it hovers */
        float fx0, fy0;
        spot551(s, hop, cw, ch, &fx0, &fy0);
        fg_colv(pal, hb + 9000.0f, 1.2f, amp * 0.5f * (1.0f - g), c);
        gk_dot(&g551, fx0 + face * L * 1.5f, fy0 + L * 0.9f, c, 2.5f * sc, 9.0f * sc, 0.35f);
    }
    gk_present(&g551, fb, w, h);
}
