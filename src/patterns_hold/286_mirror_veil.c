/* pattern_286 — MIRROR VEIL (field): a flowing veil mirrored left-right and
 * top-bottom, its folds warped by slow turbulence so they meet at the axes
 * in soft symmetric shapes; black between the folds.  2-axis mirror.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_286(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float k = vk_seedr(seed, 1, 2.5f, 4.0f);
    const float tz = t * 0.0015f;
    for (int y = 0; y < sh; y++) {
        float v = vk_absf((float)y / (float)sh - 0.5f) * 2.0f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = vk_absf((float)x / (float)sw - 0.5f) * 2.0f;
            float wx = vk_fbm3(u * 1.5f, v * 1.5f, tz, 3, seed) - 0.5f;
            float wy = vk_fbm3(u * 1.5f + 4.0f, v * 1.5f + 2.0f, tz, 3, seed ^ 5u) - 0.5f;
            float p = (u + wx * 1.2f) * k * 0.6f + (v + wy * 1.2f) * k + t * 0.002f;
            float f = vk_fract(p);
            float m = vk_sstep(0.05f, 0.30f, f) * vk_sstep(0.75f, 0.50f, f);
            float sheen = 0.5f + 0.5f * vk_sin(p * VK_TAU * 3.0f + wx * 5.0f);
            float ci = base + floorf(p) * 500.0f + (wx + wy) * 1500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, sheen, m * (0.8f + 0.2f * sheen)));
        }
    }
    vk_blit(&cv, fb, w, h);
}
