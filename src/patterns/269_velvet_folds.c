/* pattern_269 — VELVET FOLDS (field): heavy velvet gathered into deep folds,
 * light raking across the ridges, the troughs of the folds falling to black.
 * Mirror symmetry.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_269(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.0012f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = vk_absf((float)x / (float)sw - 0.5f) * 2.0f;
            /* fold height field: low-frequency noise stretched vertically */
            float hgt = vk_fbm3(u * 3.0f, v * 0.9f, tz, 3, seed);
            /* gradient in u ~ shading of a raking light from the side */
            float h2 = vk_fbm3(u * 3.0f + 0.02f, v * 0.9f, tz, 3, seed);
            float slope = (h2 - hgt) * 60.0f;
            float lit = 0.5f + slope;
            float m = vk_sstep(0.25f, 0.85f, lit) * vk_sstep(0.3f, 0.5f, hgt + 0.1f);
            float nap = 0.85f + 0.15f * vk_noise2(u * 40.0f, v * 40.0f, seed);
            float ci = base + hgt * 2600.0f + lit * 800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, lit, m * nap));
        }
    }
    vk_blit(&cv, fb, w, h);
}
