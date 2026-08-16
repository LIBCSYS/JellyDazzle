/* pattern_271 — AURORA SWIRL (field): aurora curtains folded into a
 * three-fold spiral, rays streaming outward, black sky between the arms.
 * 3-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_271(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.002f;
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v) + 1e-3f;
            float ang = atan2f(v, u) + t * 0.0004f;
            /* 3-fold: angle folded, spiral shear */
            float a3 = vk_fract(ang * 3.0f / VK_TAU + r * 0.6f);
            float arm = 0.5f + 0.5f * vk_cos(a3 * VK_TAU);
            float rays = vk_noise2(a3 * 30.0f + r * 4.0f, tz, seed);
            float rays2 = vk_noise2(a3 * 90.0f, tz * 1.5f + r * 2.0f, seed ^ 5u);
            float m = vk_sstep(0.22f, 0.7f, arm) * vk_sstep(0.22f, 0.5f, rays * 0.6f + rays2 * 0.4f) * vk_sstep(1.35f, 0.7f, r) * vk_sstep(0.05f, 0.25f, r);
            float ci = base + r * 3000.0f + rays * 900.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, rays2, m * (0.7f + 0.3f * rays2)));
        }
    }
    vk_blit(&cv, fb, w, h);
}
