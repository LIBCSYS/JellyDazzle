/* 570 Orrery — a brass-and-glass model of a system: a sun at the hub, five
 * planets on tilted circular orbits at Kepler-ish speeds (outer = slower),
 * two of them with moons, each on a thin arm from the centre.  Orbit rings
 * faint, arms dim, bodies glowing in their own drifting hues.  Figure
 * overlay, repaint. */
#include "_fig541.h"

#define NP570 5
static gk g570;

void pattern_570(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g570, w, h);
    gk_clear(&g570);
    float cw = (float)g570.cw, ch = (float)g570.ch, sc = g570.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    float cx = cw * 0.5f + cw * 0.02f * sinf(t * 0.0009f), cy = ch * 0.52f + ch * 0.02f * sinf(t * 0.0012f);
    float tilt = 0.42f + 0.10f * sinf(t * 0.0006f + gk_hash(seed) * 6.0f);     /* y squash of the orbit plane */
    float rot = 0.15f * sinf(t * 0.0005f);                                       /* slight plane roll */
    float cr = cosf(rot), sr = sinf(rot);
    float Rmax = (cw < ch ? cw : ch) * 0.46f;
    float hb = fg_pick_sat(pal, gk_hash(seed + 5u) * 32768.0f, 6000.0f);
    float c[3];
    int i, k;
    /* sun */
    fg_colv(pal, hb + 600.0f * sinf(t * 0.004f), 1.2f, amp * 0.9f, c);
    gk_dot(&g570, cx, cy, c, 6.0f * sc, 24.0f * sc, 0.4f);
    gk_col(pal, (int)hb, 0.5f, amp * 0.8f, c);
    gk_dot(&g570, cx, cy, c, 3.0f * sc, 8.0f * sc, 0.3f);
    for (i = 0; i < NP570; i++) {
        uint32_t s = seed + (uint32_t)i * 613u;
        float rad = Rmax * (0.22f + 0.78f * (float)i / (float)(NP570 - 1)) * (0.94f + 0.06f * gk_hash(s + 1u));
        float spd = 0.010f * powf(rad / (Rmax * 0.22f), -1.5f) * (gk_hash(seed + 7u) < 0.5f ? -1.0f : 1.0f);
        float a = t * spd + gk_hash(s + 2u) * GK_TAU;
        float hue = fg_pick_sat(pal, gk_hash(s + 3u) * 32768.0f, 6000.0f) + 900.0f * sinf(t * 0.004f + (float)i);
        /* orbit ring */
        fg_colv(pal, hue + 2000.0f, 1.2f, amp * 0.13f, c);
        fg_ellipse_ring(&g570, cx, cy, rad, rad * tilt, rot, c, 0.7f * sc, 2.0f * sc, 0.3f, 72);
        /* planet position on the tilted plane */
        float u = cosf(a) * rad, v = sinf(a) * rad * tilt;
        float px = cx + u * cr - v * sr, py = cy + u * sr + v * cr;
        float depth = 0.8f + 0.2f * sinf(a);              /* nearer at the bottom */
        /* arm */
        fg_colv(pal, hb + 8000.0f, 1.1f, amp * 0.28f, c);
        gk_seg(&g570, cx, cy, px, py, c, 0.7f * sc, 2.0f * sc, 0.25f);
        /* planet */
        float R = (5.0f + 4.5f * gk_hash(s + 4u)) * sc * depth;
        fg_colv(pal, hue, 1.3f, amp * 0.85f, c);
        gk_disc(&g570, px, py, R, c);
        fg_colv(pal, hue + 1200.0f, 1.2f, amp * 0.5f, c);
        gk_dot(&g570, px, py, c, R * 0.8f, R * 2.5f, 0.35f);
        /* moons on planets 2 and 4 */
        if (i == 2 || i == 4) {
            int nm = i == 2 ? 1 : 2;
            for (k = 0; k < nm; k++) {
                float mr = R * (2.6f + 1.4f * (float)k);
                float ma = t * (0.05f - 0.015f * (float)k) * (i == 2 ? 1.0f : -1.0f) + (float)k * 2.0f;
                float mu = cosf(ma) * mr, mv = sinf(ma) * mr * tilt;
                float mx = px + mu * cr - mv * sr, my = py + mu * sr + mv * cr;
                fg_colv(pal, hue + 4000.0f + (float)k * 1500.0f, 1.2f, amp * 0.6f, c);
                gk_dot(&g570, mx, my, c, 1.6f * sc, 4.0f * sc, 0.3f);
                fg_colv(pal, hue + 4000.0f, 1.2f, amp * 0.1f, c);
                fg_ellipse_ring(&g570, px, py, mr, mr * tilt, rot, c, 0.5f * sc, 1.5f * sc, 0.3f, 24);
            }
        }
    }
    /* base pillar hint */
    fg_colv(pal, hb + 8000.0f, 1.1f, amp * 0.25f, c);
    gk_seg(&g570, cx, cy, cx, cy + Rmax * 0.55f, c, 1.4f * sc, 4.0f * sc, 0.3f);
    gk_seg(&g570, cx - Rmax * 0.18f, cy + Rmax * 0.55f, cx + Rmax * 0.18f, cy + Rmax * 0.55f, c, 1.4f * sc, 4.0f * sc, 0.3f);
    gk_present(&g570, fb, w, h);
}
