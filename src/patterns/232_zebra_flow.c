/* pattern_232 — ZEBRA FLOW (field): broad stripes bent by a slow turbulent
 * flow, black stripes between the coloured ones, the whole field creeping
 * sideways.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_232(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float k = vk_seedr(seed, 1, 5.0f, 8.0f);
    const float tz = t * 0.0015f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 2.0f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 2.7f;
            float wx = vk_fbm3(u, v, tz, 3, seed) - 0.5f;
            float wy = vk_fbm3(u + 5.0f, v + 3.0f, tz, 3, seed ^ 2u) - 0.5f;
            float p = (u + wx * 1.4f) * k + (v + wy * 1.4f) * 0.5f + t * 0.003f;
            float f = vk_fract(p);
            float m = vk_sstep(0.02f, 0.20f, f) * vk_sstep(0.70f, 0.50f, f);
            float ci = base + floorf(p) * 700.0f + f * 1200.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1900.0f, wx + 0.5f, m * (0.85f + 0.15f * (wy + 0.5f))));
        }
    }
    vk_blit(&cv, fb, w, h);
}
