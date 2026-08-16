/* 550 Kites on Strings — three diamond kites bob on the wind high in the
 * frame, each tethered by a thin bowed string to a point low down; bow tails
 * of ribbon flutter behind.  The four panels of each kite take separate
 * drifting palette offsets.  Figure overlay, repaint. */
#include "_fig541.h"

#define NK550 3
static gk g550;

void pattern_550(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g550, w, h);
    gk_clear(&g550);
    float cw = (float)g550.cw, ch = (float)g550.ch, sc = g550.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NK550; i++) {
        uint32_t s = seed + (uint32_t)i * 613u;
        float ax = cw * (0.2f + 0.6f * gk_hash(s + 1u)) + cw * 0.02f * sinf(t * 0.003f + (float)i);   /* anchor */
        float ay = ch * 1.05f;
        float wind = 0.5f + 0.4f * gk_hash(s + 2u);
        float kx = ax + cw * (0.10f + 0.10f * wind) * (gk_hash(s + 3u) < 0.5f ? -1.0f : 1.0f)
                 + cw * 0.05f * sinf(t * 0.006f + gk_hash(s + 4u) * 6.0f) + cw * 0.02f * sinf(t * 0.017f + (float)i);
        float ky = ch * (0.18f + 0.22f * gk_hash(s + 5u)) + ch * 0.04f * sinf(t * 0.0045f + gk_hash(s + 6u) * 6.0f);
        float sz = (22.0f + 16.0f * gk_hash(s + 7u)) * sc;
        float tilt = 0.25f * sinf(t * 0.008f + gk_hash(s + 8u) * 6.0f) + (kx > ax ? 0.35f : -0.35f);
        float ca = cosf(tilt), sa = sinf(tilt);
        float hb = fg_pick_sat(pal, gk_hash(s + 9u) * 32768.0f, 6000.0f) + 900.0f * sinf(t * 0.004f + (float)i);
        float c[3];
        /* string: quadratic bow from anchor to kite */
        float lx = ax, ly = ay;
        fg_colv(pal, hb + 7000.0f, 1.1f, amp * 0.35f, c);
        for (k = 1; k <= 20; k++) {
            float u = (float)k / 20.0f;
            float bxp = ax + (kx - ax) * u, byp = ay + (ky - ay) * u + ch * 0.08f * u * (1.0f - u) * 4.0f * (0.5f + 0.5f * wind);
            gk_seg(&g550, lx, ly, bxp, byp, c, 0.6f * sc, 1.6f * sc, 0.25f);
            lx = bxp; ly = byp;
        }
        /* diamond: top, right, bottom, left about (kx,ky) */
        float tx, ty, rx, ry, bx, by, wx, wy;
        fg_xf(0.0f, -sz, ca, sa, kx, ky, &tx, &ty);
        fg_xf(sz * 0.7f, -sz * 0.2f, ca, sa, kx, ky, &rx, &ry);
        fg_xf(0.0f, sz * 1.1f, ca, sa, kx, ky, &bx, &by);
        fg_xf(-sz * 0.7f, -sz * 0.2f, ca, sa, kx, ky, &wx, &wy);
        float cxx, cyy; fg_xf(0.0f, -sz * 0.2f, ca, sa, kx, ky, &cxx, &cyy);
        fg_colv(pal, hb, 1.3f, amp * 0.55f, c);            fg_tri(&g550, tx, ty, rx, ry, cxx, cyy, c);
        fg_colv(pal, hb + 2000.0f, 1.3f, amp * 0.55f, c);  fg_tri(&g550, rx, ry, bx, by, cxx, cyy, c);
        fg_colv(pal, hb + 4000.0f, 1.3f, amp * 0.55f, c);  fg_tri(&g550, bx, by, wx, wy, cxx, cyy, c);
        fg_colv(pal, hb + 6000.0f, 1.3f, amp * 0.55f, c);  fg_tri(&g550, wx, wy, tx, ty, cxx, cyy, c);
        /* spars */
        fg_colv(pal, hb + 8000.0f, 1.1f, amp * 0.7f, c);
        gk_seg(&g550, tx, ty, bx, by, c, 0.8f * sc, 2.0f * sc, 0.3f);
        gk_seg(&g550, wx, wy, rx, ry, c, 0.8f * sc, 2.0f * sc, 0.3f);
        /* tail ribbon: sine wave hanging from bottom, blown downwind */
        float px = bx, py = by;
        for (k = 1; k <= 12; k++) {
            float u = (float)k / 12.0f;
            float dxs = -sa * sz * 0.35f, dys = ca * sz * 0.35f;         /* down along kite axis */
            float wig = sinf(t * 0.03f + (float)k * 0.9f) * sz * 0.22f;
            float nx = bx + dxs * (float)k * 0.9f + ca * wig, ny = by + dys * (float)k * 0.9f + sa * wig;
            fg_colv(pal, hb + 1000.0f + u * 6000.0f, 1.3f, amp * 0.6f * (1.0f - 0.5f * u), c);
            gk_seg(&g550, px, py, nx, ny, c, 0.9f * sc, 2.5f * sc, 0.3f);
            if (k % 3 == 0) gk_dot(&g550, nx, ny, c, 1.8f * sc, 4.0f * sc, 0.3f);
            px = nx; py = ny;
        }
    }
    gk_present(&g550, fb, w, h);
}
