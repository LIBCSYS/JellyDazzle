/* pattern_251 — STAR LATTICE (field): an eight-pointed star tiling in the
 * girih manner, stars glowing, the cross-shaped gaps between them black,
 * the whole tessellation breathing.  8-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_251(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 3.5f, 5.5f);
    const float rot = vk_seedr(seed, 2, 0.0f, 0.4f) + 0.02f * vk_sin(t * 0.001f);
    const float cr = vk_cos(rot), sr = vk_sin(rot);
    for (int y = 0; y < sh; y++) {
        float v0 = ((float)y / (float)sh - 0.5f) * 1.5f * n;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u0 = ((float)x / (float)sw - 0.5f) * 2.0f * n;
            float u = u0 * cr - v0 * sr, v = u0 * sr + v0 * cr;
            /* nearest lattice point (square lattice) */
            float fu = vk_fract(u) - 0.5f, fv = vk_fract(v) - 0.5f;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float ang = atan2f(fv, fu), r = sqrtf(fu * fu + fv * fv);
            /* 8-pointed star: radius modulated by cos(8*ang) */
            float breathe = 0.05f * vk_sin(t * 0.004f + vk_h2(iu, iv, seed) * VK_TAU);
            float star_r = 0.36f + 0.14f * vk_cos(ang * 8.0f) + breathe;
            float star = vk_sstep(star_r, star_r - 0.06f, r);
            /* inner octagon glow + edge line */
            float inner = vk_sstep(0.22f, 0.05f, r) * 0.5f;
            float edge = vk_sstep(0.05f, 0.015f, vk_absf(r - star_r));
            float m = star * (0.55f + inner) ;
            m = m > edge ? m : edge;
            float ci = base + vk_h2(iu, iv, seed ^ 3u) * 1600.0f + r * 1800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, 0.5f + 0.5f * vk_cos(ang * 8.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
