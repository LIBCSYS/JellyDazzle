/* pattern_220 — HONEYCOMB BREATH (field): a hex comb whose cells swell and
 * shrink on their own slow clocks; the comb walls and the emptied cells are
 * black.  6-fold lattice.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_220(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float sc = vk_seedr(seed, 1, 6.0f, 10.0f);
    const float rot = vk_seedr(seed, 2, 0.0f, 0.5f);
    const float cr = vk_cos(rot), sr = vk_sin(rot);
    for (int y = 0; y < sh; y++) {
        float v0 = ((float)y / (float)sh - 0.5f) * 1.5f * sc;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u0 = ((float)x / (float)sw - 0.5f) * 2.0f * sc;
            float u = u0 * cr - v0 * sr, v = u0 * sr + v0 * cr;
            /* hex grid via two candidate centres */
            float qx = u, qy = v / 0.8660254f;
            float ax = floorf(qx), ay = floorf(qy);
            float bestd = 9.0f, cx = 0, cy = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                float gy = ay + j, gx = ax + i + (((int)gy & 1) ? 0.5f : 0.0f);
                float hx = gx, hy = gy * 0.8660254f;
                float dx = u - hx, dy = v - hy, d = dx * dx + dy * dy;
                if (d < bestd) { bestd = d; cx = hx; cy = hy; }
            }
            float dx = u - cx, dy = v - cy;
            /* hex distance */
            float hd = vk_absf(dx) * 0.8660254f + vk_absf(dy) * 0.5f;
            if (vk_absf(dy) > hd) hd = vk_absf(dy);
            int ix = (int)floorf(cx * 2.0f + 100.0f), iy = (int)floorf(cy * 2.0f + 100.0f);
            float ph = vk_h2(ix, iy, seed) * VK_TAU;
            float open = 0.5f + 0.5f * vk_sin(t * 0.004f + ph);
            float radius = 0.44f * vk_sstep(0.15f, 0.7f, open);
            float m = vk_sstep(radius, radius * 0.6f, hd) * (0.5f + 0.5f * open);
            float ci = base + vk_h2(ix, iy, seed ^ 9u) * 2400.0f + hd * 2000.0f + t * 0.7f;
            float cj = ci + 1500.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, open, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
