/* pattern_209 — SMOKE FIELD (field): slow rolling smoke, dense billows lit
 * in colour, thin air between them black.  Domain-warped fbm.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_209(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.0025f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 2.6f + tz * 0.8f;   /* smoke rises */
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 3.5f;
            float wx = vk_fbm3(u * 0.7f, v * 0.7f, tz, 2, seed ^ 3u) - 0.5f;
            float wy = vk_fbm3(u * 0.7f + 9.0f, v * 0.7f, tz, 2, seed ^ 4u) - 0.5f;
            float d = vk_fbm3(u + wx * 1.6f, v + wy * 1.6f, tz * 1.3f, 4, seed);
            float m = vk_sstep(0.42f, 0.70f, d);
            float lit = 0.5f + 0.4f * vk_sstep(0.5f, 0.85f, d);
            float ci = base + d * 3200.0f + wx * 1200.0f + t * 0.5f;
            float cj = ci + 2000.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, wy + 0.5f, m * lit));
        }
    }
    vk_blit(&cv, fb, w, h);
}
