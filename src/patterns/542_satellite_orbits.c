/* 542 Satellite Orbits — a soft central body with three satellites on
 * tilted orbits (3-D circles projected as the camera slowly turns), faint
 * orbit paths, short trailing arcs.  Each satellite carries its own drifting
 * hue.  Figure overlay, repaint. */
#include "_fig541.h"

#define NS542 3
static gk g542;

void pattern_542(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g542, w, h);
    gk_clear(&g542);
    float cw = (float)g542.cw, ch = (float)g542.ch, sc = g542.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    gk_cam cam;
    float ay = t * 0.0011f + gk_hash(seed) * GK_TAU;
    float ax = 0.5f + 0.35f * sinf(t * 0.0007f + gk_hash(seed + 1u) * 6.0f);
    float cx = cw * 0.5f + cw * 0.04f * sinf(t * 0.0009f), cy = ch * 0.5f + ch * 0.04f * cosf(t * 0.0012f);
    float R0 = (cw < ch ? cw : ch) * 0.16f;
    gk_cam_set(&cam, &g542, ay, ax, 4.5f, R0 / sc);   /* model units = R0 */
    cam.cx = cx; cam.cy = cy;
    float hb = fg_pick_sat(pal, gk_hash(seed + 5u) * 32768.0f, 6000.0f);
    float c[3];
    /* central body */
    fg_colv(pal, hb + 800.0f * sinf(t * 0.005f), 1.3f, amp * 0.55f, c);
    gk_disc(&g542, cx, cy, R0 * 0.55f, c);
    fg_colv(pal, hb + 3000.0f, 1.2f, amp * 0.5f, c);
    gk_ring(&g542, cx, cy, R0 * 0.55f, 3.0f * sc, c);
    int i, k;
    for (i = 0; i < NS542; i++) {
        float tilt = 0.4f + 1.1f * gk_hash(seed + 20u + (uint32_t)i);
        float node = gk_hash(seed + 30u + (uint32_t)i) * GK_TAU + t * 0.0004f * (float)(i + 1);
        float rad = R0 * (0.85f + 0.5f * (float)i);
        float spd = (0.011f - 0.0025f * (float)i) * (gk_hash(seed + 40u + (uint32_t)i) < 0.5f ? -1.0f : 1.0f);
        float ph = t * spd + gk_hash(seed + 50u + (uint32_t)i) * GK_TAU;
        float hue = fg_pick_sat(pal, gk_hash(seed + 60u + (uint32_t)i) * 32768.0f, 6000.0f) + 1500.0f * sinf(t * 0.003f + (float)i);
        float ct = cosf(tilt), st = sinf(tilt), cn = cosf(node), sn = sinf(node);
        /* orbit path */
        float lx = 0.0f, ly = 0.0f;
        fg_colv(pal, hue + 2000.0f, 1.2f, amp * 0.16f, c);
        for (k = 0; k <= 64; k++) {
            float a = GK_TAU * (float)k / 64.0f;
            float x = cosf(a) * rad, z = sinf(a) * rad, y = 0.0f;
            /* tilt about x, then rotate about y by node */
            float y1 = y * ct - z * st, z1 = y * st + z * ct;
            float x2 = x * cn + z1 * sn, z2 = -x * sn + z1 * cn;
            float sx, sy;
            gk_project(&cam, x2 / R0, y1 / R0, z2 / R0, &sx, &sy, NULL);
            if (k) gk_seg(&g542, lx, ly, sx, sy, c, 0.8f * sc, 2.5f * sc, 0.3f);
            lx = sx; ly = sy;
        }
        /* satellite + trailing arc */
        for (k = 10; k >= 0; k--) {
            float a = ph - (float)k * 0.045f;
            float x = cosf(a) * rad, z = sinf(a) * rad, y = 0.0f;
            float y1 = y * ct - z * st, z1 = y * st + z * ct;
            float x2 = x * cn + z1 * sn, z2 = -x * sn + z1 * cn;
            float sx, sy, dp;
            gk_project(&cam, x2 / R0, y1 / R0, z2 / R0, &sx, &sy, &dp);
            float f = 1.0f - (float)k / 11.0f;
            if (k == 0) {
                fg_colv(pal, hue, 1.3f, amp * 0.9f * dp, c);
                gk_dot(&g542, sx, sy, c, 5.0f * sc * dp, 14.0f * sc, 0.35f);
                gk_col(pal, (int)hue, 0.5f, amp * 0.9f, c);
                gk_dot(&g542, sx, sy, c, 2.5f * sc * dp, 6.0f * sc, 0.2f);
            } else {
                fg_colv(pal, hue + (float)k * 250.0f, 1.3f, amp * 0.5f * f * f, c);
                gk_dot(&g542, sx, sy, c, 2.0f * sc * f * dp, 6.0f * sc, 0.25f);
            }
        }
    }
    gk_present(&g542, fb, w, h);
}
