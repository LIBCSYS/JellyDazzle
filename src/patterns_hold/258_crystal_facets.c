/* pattern_258 — CRYSTAL FACETS (field): a bed of hexagonal crystal prisms
 * seen end-on, each prism's faces lit or dark as the light swings; black
 * shadowed faces let the ground through.  6-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_258(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float sc = vk_seedr(seed, 1, 4.0f, 6.0f);
    const float la = t * 0.002f;
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f * sc;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f * sc;
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
            int ix = (int)floorf(cx * 2.0f + 100.0f), iy = (int)floorf(cy * 2.0f + 100.0f);
            float hgt = vk_h2(ix, iy, seed);
            float ang = atan2f(dy, dx);
            /* six pyramid faces meeting at the centre */
            float fi = floorf((ang + 3.14159f) / (VK_TAU / 6.0f));
            float fang = (fi + 0.5f) * (VK_TAU / 6.0f) - 3.14159f;
            float lit = 0.5f + 0.5f * vk_cos(fang - la - hgt * 2.0f);
            float hd = vk_absf(dx) * 0.8660254f + vk_absf(dy) * 0.5f;
            if (vk_absf(dy) > hd) hd = vk_absf(dy);
            float edge = vk_sstep(0.5f, 0.46f, hd);
            float apex = 0.7f + 0.3f * (1.0f - hd * 2.0f);
            float m = vk_sstep(0.25f, 0.6f, lit) * edge * apex;
            float ci = base + hgt * 1800.0f + lit * 1200.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, lit, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
