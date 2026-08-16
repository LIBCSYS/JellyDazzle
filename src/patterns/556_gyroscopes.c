/* 556 Gyroscopes — three gyroscopes: an outer gimbal ring, an inner ring
 * turning inside it on a perpendicular axis, and a rotor disc spinning
 * inside that, the whole thing precessing slowly.  Rings drawn as projected
 * 3-D circles with hue running round them; the rotor a foreshortened disc.
 * Figure overlay, repaint. */
#include "_fig541.h"

#define NG556 3
static gk g556;

/* rotate (x,y,z) about x by a, then about y by b, then about z by c */
static inline void rot556(float *x, float *y, float *z, float ca, float sa, float cb, float sb, float cc, float sc_)
{
    float y1 = *y * ca - *z * sa, z1 = *y * sa + *z * ca;
    float x2 = *x * cb + z1 * sb, z2 = -*x * sb + z1 * cb;
    float x3 = x2 * cc - y1 * sc_, y3 = x2 * sc_ + y1 * cc;
    *x = x3; *y = y3; *z = z2;
}
static void ring556(gk *g, float ox, float oy, float S, float R, int axis, float a, float b, float c_,
                    float wobble_a, float hb, float amp, float sc, const uint32_t *pal, float wd)
{
    float ca = cosf(a), sa = sinf(a), cb = cosf(b), sb = sinf(b), cc = cosf(c_), sc_ = sinf(c_);
    float cw_ = cosf(wobble_a), sw_ = sinf(wobble_a);
    float lx = 0.0f, ly = 0.0f, col[3];
    int k;
    for (k = 0; k <= 56; k++) {
        float u = GK_TAU * (float)k / 56.0f;
        float x, y, z;
        if (axis == 0) { x = 0.0f; y = cosf(u) * R; z = sinf(u) * R; }
        else if (axis == 1) { x = cosf(u) * R; y = 0.0f; z = sinf(u) * R; }
        else { x = cosf(u) * R; y = sinf(u) * R; z = 0.0f; }
        /* inner rotation about the ring's own hinge (x axis) by wobble */
        { float y1 = y * cw_ - z * sw_, z1 = y * sw_ + z * cw_; y = y1; z = z1; }
        rot556(&x, &y, &z, ca, sa, cb, sb, cc, sc_);
        float px = ox + x * S, py = oy + y * S;
        if (k) {
            float sh = 0.55f + 0.45f * fg_clamp01(0.5f + z * 0.5f);
            fg_colv(pal, hb + 1800.0f * sinf(u * 2.0f + a * 3.0f), 1.3f, amp * sh, col);
            gk_seg(g, lx, ly, px, py, col, wd * sc, wd * 3.0f * sc, 0.35f);
        }
        lx = px; ly = py;
    }
}

void pattern_556(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g556, w, h);
    gk_clear(&g556);
    float cw = (float)g556.cw, ch = (float)g556.ch, sc = g556.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NG556; i++) {
        uint32_t s = seed + (uint32_t)i * 761u;
        float ox = cw * (0.2f + 0.3f * (float)i) + cw * 0.04f * sinf(t * 0.0019f + gk_hash(s + 1u) * 6.0f);
        float oy = ch * (0.3f + 0.4f * gk_hash(s + 2u)) + ch * 0.04f * sinf(t * 0.0023f + gk_hash(s + 3u) * 6.0f);
        float S = (40.0f + 22.0f * gk_hash(s + 4u)) * sc;
        /* frame orientation precesses */
        float a = 0.5f + 0.25f * sinf(t * 0.0021f + gk_hash(s + 5u) * 6.0f);
        float b = t * (0.0025f + 0.002f * gk_hash(s + 6u)) + gk_hash(s + 7u) * 6.0f;
        float c_ = 0.2f * sinf(t * 0.0017f + (float)i);
        float in1 = t * (0.008f + 0.004f * gk_hash(s + 8u));         /* inner gimbal swing */
        float in2 = t * (0.03f + 0.01f * gk_hash(s + 9u));           /* rotor spin */
        float hb = fg_pick_sat(pal, gk_hash(s + 10u) * 32768.0f, 6000.0f) + 900.0f * sinf(t * 0.004f + (float)i);
        /* outer ring: circle in yz-plane (axis x), fixed to frame */
        ring556(&g556, ox, oy, S, 1.0f, 2, a, b, c_, 0.0f, hb, amp * 0.85f, sc, pal, 1.4f);
        /* inner ring: circle about the y axis, swinging about x by in1 */
        ring556(&g556, ox, oy, S, 0.72f, 1, a, b, c_, 0.6f * sinf(in1), hb + 3000.0f, amp * 0.85f, sc, pal, 1.2f);
        /* rotor: disc in the inner ring's plane, drawn as filled polygon of 24 pts + spokes */
        {
            float ca = cosf(a), sa = sinf(a), cb = cosf(b), sb = sinf(b), cc = cosf(c_), sc_ = sinf(c_);
            float wa = 0.6f * sinf(in1), cw_ = cosf(wa), sw_ = sinf(wa);
            float xs[16], ys[16], col[3];
            for (k = 0; k < 16; k++) {
                float u = GK_TAU * (float)k / 16.0f;
                float x = cosf(u) * 0.5f, y = 0.0f, z = sinf(u) * 0.5f;
                { float y1 = y * cw_ - z * sw_, z1 = y * sw_ + z * cw_; y = y1; z = z1; }
                rot556(&x, &y, &z, ca, sa, cb, sb, cc, sc_);
                xs[k] = ox + x * S; ys[k] = oy + y * S;
            }
            fg_colv(pal, hb + 6000.0f, 1.3f, amp * 0.28f, col);
            fg_poly_fill(&g556, xs, ys, 16, col);
            /* rotor spokes, spinning */
            for (k = 0; k < 4; k++) {
                float u = in2 + GK_TAU * (float)k / 4.0f;
                float x = cosf(u) * 0.48f, y = 0.0f, z = sinf(u) * 0.48f;
                float x0 = -x, y0 = 0.0f, z0 = -z;
                { float y1 = y * cw_ - z * sw_, z1 = y * sw_ + z * cw_; y = y1; z = z1; }
                { float y1 = y0 * cw_ - z0 * sw_, z1 = y0 * sw_ + z0 * cw_; y0 = y1; z0 = z1; }
                rot556(&x, &y, &z, ca, sa, cb, sb, cc, sc_);
                rot556(&x0, &y0, &z0, ca, sa, cb, sb, cc, sc_);
                fg_colv(pal, hb + 7500.0f + (float)k * 500.0f, 1.2f, amp * 0.5f, col);
                gk_seg(&g556, ox + x0 * S, oy + y0 * S, ox + x * S, oy + y * S, col, 0.8f * sc, 2.2f * sc, 0.3f);
            }
            /* axle through the rotor along the inner ring's axis (y after wobble) */
            float x = 0.0f, y = 0.9f, z = 0.0f, x0 = 0.0f, y0 = -0.9f, z0 = 0.0f;
            { float y1 = y * cw_ - z * sw_, z1 = y * sw_ + z * cw_; y = y1; z = z1; }
            { float y1 = y0 * cw_ - z0 * sw_, z1 = y0 * sw_ + z0 * cw_; y0 = y1; z0 = z1; }
            rot556(&x, &y, &z, ca, sa, cb, sb, cc, sc_);
            rot556(&x0, &y0, &z0, ca, sa, cb, sb, cc, sc_);
            fg_colv(pal, hb + 9000.0f, 1.1f, amp * 0.6f, col);
            gk_seg(&g556, ox + x0 * S, oy + y0 * S, ox + x * S, oy + y * S, col, 1.0f * sc, 2.5f * sc, 0.3f);
            gk_col(pal, (int)(hb + 9000.0f), 0.5f, amp * 0.8f, col);
            gk_dot(&g556, ox, oy, col, 1.8f * sc, 5.0f * sc, 0.3f);
        }
    }
    gk_present(&g556, fb, w, h);
}
