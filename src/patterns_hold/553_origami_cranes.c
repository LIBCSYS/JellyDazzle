/* 553 Origami Cranes — three paper cranes turn slowly in 3-D (about their
 * vertical axis, with a gentle nod), wings raised and lowering on a slow
 * breath, while drifting.  Each facet of the fold is its own palette panel;
 * hues drift per crane.  Figure overlay, repaint. */
#include "_fig541.h"

#define NC553 3
static gk g553;

/* rotate a model point about y by (cy,sy), then tilt about x by (cx,sx),
 * orthographic to canvas with scale S about (ox,oy) */
static inline void pt553(float x, float y, float z, float cy, float sy, float cx, float sx,
                         float S, float ox, float oy, float *px, float *py, float *pz)
{
    float x1 = x * cy + z * sy, z1 = -x * sy + z * cy;
    float y2 = y * cx - z1 * sx, z2 = y * sx + z1 * cx;
    *px = ox + x1 * S; *py = oy + y2 * S; *pz = z2;
}

void pattern_553(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g553, w, h);
    gk_clear(&g553);
    float cw = (float)g553.cw, ch = (float)g553.ch, sc = g553.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i;
    for (i = 0; i < NC553; i++) {
        uint32_t s = seed + (uint32_t)i * 503u;
        float ox = cw * (0.2f + 0.3f * (float)i) + cw * 0.06f * sinf(t * 0.0017f + gk_hash(s + 1u) * 6.0f);
        float oy = ch * (0.35f + 0.3f * gk_hash(s + 2u)) + ch * 0.05f * sinf(t * 0.0023f + gk_hash(s + 3u) * 6.0f);
        float S = (44.0f + 18.0f * gk_hash(s + 4u)) * sc;
        float yaw = t * (0.004f + 0.003f * gk_hash(s + 5u)) * (gk_hash(s + 6u) < 0.5f ? -1.0f : 1.0f) + gk_hash(s + 7u) * 6.0f;
        float tilt = 0.35f + 0.15f * sinf(t * 0.003f + (float)i);
        float cy = cosf(yaw), sy = sinf(yaw), cx = cosf(tilt), sx = sinf(tilt);
        float wing = 0.35f + 0.5f * (0.5f + 0.5f * sinf(t * 0.006f + gk_hash(s + 8u) * 6.0f));  /* wing raise */
        float hb = fg_pick_sat(pal, gk_hash(s + 9u) * 32768.0f, 6000.0f) + 900.0f * sinf(t * 0.004f + (float)i);
        float c[3];
        /* model (x right, y up, z toward viewer): body diamond in xz-plane at y=0 */
        /* body points */
        float bx[4] = { 0.0f, 0.55f, 0.0f, -0.55f }, by_[4] = { 0.15f, 0.0f, 0.15f, 0.0f }, bz[4] = { 0.5f, 0.0f, -0.5f, 0.0f };
        /* screen coordinates */
        float px[8], py[8], pz[8];
        int k;
        for (k = 0; k < 4; k++) pt553(bx[k], -by_[k], bz[k], cy, sy, cx, sx, S, ox, oy, &px[k], &py[k], &pz[k]);
        /* body: two triangles (front and back halves) */
        fg_colv(pal, hb, 1.3f, amp * 0.5f, c);
        fg_tri(&g553, px[0], py[0], px[1], py[1], px[2], py[2], c);
        fg_colv(pal, hb + 1200.0f, 1.3f, amp * 0.5f, c);
        fg_tri(&g553, px[0], py[0], px[2], py[2], px[3], py[3], c);
        /* wings: from spine (0,0.15,±0.2) out to tips (±1.4, wing, 0) */
        float wx, wy, wz, ax, ay, az, bx2, by2, bz2;
        for (k = -1; k <= 1; k += 2) {
            float f = (float)k;
            pt553(f * 0.15f, -0.15f, 0.25f, cy, sy, cx, sx, S, ox, oy, &ax, &ay, &az);
            pt553(f * 0.15f, -0.15f, -0.25f, cy, sy, cx, sx, S, ox, oy, &bx2, &by2, &bz2);
            pt553(f * 1.5f, -wing, 0.0f, cy, sy, cx, sx, S, ox, oy, &wx, &wy, &wz);
            float depth = 0.75f + 0.25f * fg_clamp01(wz + 0.5f);
            fg_colv(pal, hb + 2600.0f + (k > 0 ? 1300.0f : 0.0f), 1.3f, amp * 0.5f * depth, c);
            fg_tri(&g553, ax, ay, bx2, by2, wx, wy, c);
            fg_colv(pal, hb + 5000.0f, 1.2f, amp * 0.55f, c);
            gk_seg(&g553, ax, ay, wx, wy, c, 0.7f * sc, 2.0f * sc, 0.3f);
            gk_seg(&g553, bx2, by2, wx, wy, c, 0.7f * sc, 2.0f * sc, 0.3f);
        }
        /* neck + head: from front tip (0,0.15,0.5) up-forward */
        float nx, ny, nz, hx, hy, hz, kx, ky, kz;
        pt553(0.0f, -0.6f, 0.95f, cy, sy, cx, sx, S, ox, oy, &nx, &ny, &nz);
        pt553(0.0f, -0.55f, 1.25f, cy, sy, cx, sx, S, ox, oy, &hx, &hy, &hz);
        pt553(0.0f, -0.35f, 0.75f, cy, sy, cx, sx, S, ox, oy, &kx, &ky, &kz);
        fg_colv(pal, hb + 3800.0f, 1.3f, amp * 0.5f, c);
        fg_tri(&g553, px[0], py[0], nx, ny, kx, ky, c);
        fg_tri(&g553, nx, ny, hx, hy, kx, ky, c);
        /* tail: from back tip up-back */
        float tx, ty, tz, ux, uy, uz;
        pt553(0.0f, -0.55f, -1.05f, cy, sy, cx, sx, S, ox, oy, &tx, &ty, &tz);
        pt553(0.0f, -0.25f, -0.7f, cy, sy, cx, sx, S, ox, oy, &ux, &uy, &uz);
        fg_colv(pal, hb + 6200.0f, 1.3f, amp * 0.5f, c);
        fg_tri(&g553, px[2], py[2], tx, ty, ux, uy, c);
        /* spine crease */
        fg_colv(pal, hb + 7500.0f, 1.1f, amp * 0.6f, c);
        gk_seg(&g553, px[0], py[0], px[2], py[2], c, 0.6f * sc, 1.8f * sc, 0.3f);
        /* eye glint */
        gk_col(pal, (int)(hb + 8000.0f), 0.6f, amp * 0.6f, c);
        gk_dot(&g553, hx * 0.7f + nx * 0.3f, hy * 0.7f + ny * 0.3f, c, 0.8f * sc, 2.0f * sc, 0.2f);
    }
    gk_present(&g553, fb, w, h);
}
