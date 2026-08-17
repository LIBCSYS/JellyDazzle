/* pattern_244 — KITE TILES (field): a rhombille tiling of tumbling blocks —
 * three faces per block lit differently, one face at a time dropping to
 * black as the light swings, so holes wander through the tiling.  6-fold.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_244(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float sc = vk_seedr(seed, 1, 5.0f, 8.0f);
    const float la = t * 0.003f;                     /* light angle */
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f * sc;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f * sc;
            /* triangular lattice coordinates */
            float a = u + v * 0.57735f, b = v * 1.1547f, c = a - b;   /* not used: kept simple */
            (void)c;
            /* nearest hex centre */
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
            /* which of the three rhombi: sector by angle */
            float ang = atan2f(dy, dx);
            int face = (int)floorf((ang + 3.14159f) / (VK_TAU / 3.0f) + 0.5f) % 3;
            float fang = face * (VK_TAU / 3.0f) - 3.14159f;
            /* light per face */
            float lit = 0.5f + 0.5f * vk_cos(fang - la);
            /* fade near hex centre & edges (bevel) */
            float hd = vk_absf(dx) * 0.8660254f + vk_absf(dy) * 0.5f;
            if (vk_absf(dy) > hd) hd = vk_absf(dy);
            float bevel = vk_sstep(0.5f, 0.42f, hd);
            /* black face when unlit */
            float m = vk_sstep(0.20f, 0.55f, lit) * bevel * (0.6f + 0.4f * lit);
            /* per-block colour with slow drift */
            int ix = (int)floorf(cx * 2.0f + 100.0f), iy = (int)floorf(cy * 2.0f + 100.0f);
            float ci = base + vk_h2(ix, iy, seed) * 1200.0f + face * 700.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, lit, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
