/* pattern_308 — BRAIN CORAL (field): the meandering ridges of a brain
 * coral — labyrinthine bright ridges with black grooves between, the
 * whole surface swelling gently.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_308(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.001f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 4.0f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 5.3f;
            /* labyrinth: sin of a warped coordinate = parallel meanders */
            float wx = vk_fbm3(u * 0.5f, v * 0.5f, tz, 3, seed) - 0.5f;
            float wy = vk_fbm3(u * 0.5f + 3.0f, v * 0.5f + 1.0f, tz, 3, seed ^ 5u) - 0.5f;
            float p = (u + wx * 5.0f) * 1.3f + (v + wy * 5.0f) * 0.9f;
            float s = 0.5f + 0.5f * vk_sin(p * VK_TAU * 0.5f);
            float ridge = vk_sstep(0.25f, 0.55f, s);
            float crest = 0.6f + 0.4f * vk_sstep(0.55f, 1.0f, s);
            /* dome: brighter in the middle of the frame */
            float dome = 0.6f + 0.4f * (1.0f - (u / 5.3f - 0.5f) * (u / 5.3f - 0.5f) * 2.0f - (v / 4.0f - 0.5f) * (v / 4.0f - 0.5f) * 2.0f);
            float m = ridge * crest * dome;
            float ci = base + wx * 3000.0f + s * 800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, s, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
