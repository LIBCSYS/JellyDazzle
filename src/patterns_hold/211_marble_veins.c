/* pattern_211 — MARBLE VEINS (field): marbled paper — combed veins swirl
 * across the sheet, black between the veins.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_211(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.0018f;
    const float freq = vk_seedr(seed, 2, 5.0f, 9.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 2.0f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 2.7f;
            float n = vk_fbm3(u, v, tz, 4, seed);
            float p = (u + v * 0.4f) * freq + n * 6.0f + t * 0.002f;
            float s = 0.5f + 0.5f * vk_sin(p);
            float m = vk_sstep(0.25f, 0.55f, s) * vk_sstep(0.98f, 0.80f, s);
            float sub = 0.5f + 0.5f * vk_sin(p * 4.0f + n * 3.0f);
            float ci = base + s * 1500.0f + n * 2600.0f + t * 0.5f;
            float cj = ci + 1900.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, sub, m * (0.75f + 0.25f * sub)));
        }
    }
    vk_blit(&cv, fb, w, h);
}
