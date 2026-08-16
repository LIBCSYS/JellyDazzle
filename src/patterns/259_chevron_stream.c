/* pattern_259 — CHEVRON STREAM (field): nested chevrons flowing downstream
 * like ripples in a fast channel, each band lit on its leading edge, black
 * between bands.  Mirror symmetry.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_259(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float k = vk_seedr(seed, 1, 6.0f, 10.0f);
    const float slope = vk_seedr(seed, 2, 0.5f, 1.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = vk_absf((float)x / (float)sw - 0.5f) * 2.0f;
            float wob = 0.06f * vk_sin(u * 5.0f + t * 0.002f) + 0.04f * vk_noise2(u * 3.0f, v * 3.0f + t * 0.001f, seed);
            float p = (v + u * slope + wob) * k - t * 0.003f;
            float f = vk_fract(p);
            float band = vk_sstep(0.05f, 0.30f, f) * vk_sstep(0.70f, 0.50f, f);
            float lead = vk_sstep(0.05f, 0.15f, f) * vk_sstep(0.35f, 0.15f, f);
            float m = band * (0.7f + 0.3f * lead);
            float ci = base + floorf(p) * 450.0f + f * 800.0f + u * 600.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, lead, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
