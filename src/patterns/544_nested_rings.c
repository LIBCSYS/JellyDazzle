/* 544 Nested Rings — six concentric rings, each tumbling in 3-D on its own
 * axis so they read as an armillary sphere: projected ellipses that open
 * and close, colour running round each ring and drifting between rings.
 * The whole set breathes and drifts.  Figure overlay, repaint. */
#include "_fig541.h"

#define NR544 6
static gk g544;

void pattern_544(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g544, w, h);
    gk_clear(&g544);
    float cw = (float)g544.cw, ch = (float)g544.ch, sc = g544.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    float cx = cw * 0.5f + cw * 0.05f * sinf(t * 0.0013f + gk_hash(seed) * 6.0f);
    float cy = ch * 0.5f + ch * 0.05f * sinf(t * 0.0009f + gk_hash(seed + 1u) * 6.0f);
    float R = (cw < ch ? cw : ch) * (0.30f + 0.02f * sinf(t * 0.003f));
    int i, k;
    for (i = 0; i < NR544; i++) {
        uint32_t s = seed + (uint32_t)i * 71u;
        float rad = R * (0.35f + 0.65f * (float)i / (float)(NR544 - 1));
        /* ring lies in a plane; plane normal precesses: tilt about x by a(t), spin about y by b(t) */
        float a = t * (0.0016f + 0.0012f * gk_hash(s + 2u)) + gk_hash(s + 3u) * GK_TAU;
        float b = t * (0.0011f + 0.0010f * gk_hash(s + 4u)) * (gk_hash(s + 5u) < 0.5f ? -1.0f : 1.0f) + gk_hash(s + 6u) * GK_TAU;
        float ca = cosf(a), sa = sinf(a), cb = cosf(b), sb = sinf(b);
        float hb = fg_pick_sat(pal, gk_hash(s + 7u) * 32768.0f, 6000.0f) + 1000.0f * sinf(t * 0.003f + (float)i * 1.3f);
        float c[3];
        float lx = 0.0f, ly = 0.0f, ld = 1.0f;
        int segs = 72;
        for (k = 0; k <= segs; k++) {
            float u = GK_TAU * (float)k / (float)segs;
            float x = cosf(u) * rad, y = sinf(u) * rad, z = 0.0f;
            float y1 = y * ca - z * sa, z1 = y * sa + z * ca;
            float x2 = x * cb + z1 * sb, z2 = -x * sb + z1 * cb;
            float dp = 1.0f + z2 / (R * 3.5f);        /* mild perspective + depth shade */
            float sx = cx + x2 * dp, sy = cy + y1 * dp;
            if (k) {
                float sh = 0.55f + 0.45f * fg_clamp01((dp - 0.75f) * 2.0f);
                fg_colv(pal, hb + 2200.0f * sinf(u * 2.0f + t * 0.01f + (float)i), 1.3f, amp * 0.85f * sh, c);
                gk_seg(&g544, lx, ly, sx, sy, c, 1.5f * sc * ld, 4.5f * sc, 0.35f);
            }
            lx = sx; ly = sy; ld = dp;
        }
    }
    /* core */
    float c[3];
    float hb = fg_pick_sat(pal, gk_hash(seed + 99u) * 32768.0f, 6000.0f);
    fg_colv(pal, hb + 800.0f * sinf(t * 0.004f), 1.2f, amp * 0.8f, c);
    gk_dot(&g544, cx, cy, c, 4.0f * sc, 16.0f * sc, 0.3f);
    gk_present(&g544, fb, w, h);
}
