/* pattern_313 — PLUME SPIRAL (field): five arms of soft plumes curling
 * out from the centre in a slow spiral, barbs shimmering along each plume;
 * black between the arms.  5-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_313(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float curl = vk_seedr(seed, 1, 2.0f, 3.5f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u) + t * 0.0005f + r * curl;
            float a = ang * 5.0f / VK_TAU;
            float f = vk_fract(a) - 0.5f;
            /* plume: soft body, feathery edge, barbs */
            float edge = 0.08f * vk_noise2(r * 8.0f + floorf(a) * 3.0f, t * 0.002f, seed);
            float halfw = (0.30f + edge) * vk_sstep(0.05f, 0.35f, r) * vk_sstep(1.35f, 0.8f, r);
            float body = vk_sstep(halfw, halfw * 0.2f, vk_absf(f));
            float barb = 0.7f + 0.3f * vk_sin(f * 30.0f + r * 20.0f - t * 0.006f);
            float shaft = 0.8f + 0.2f * vk_sstep(0.03f, 0.0f, vk_absf(f));
            float m = body * barb * shaft * vk_sstep(0.03f, 0.15f, r);
            float ci = base + r * 2000.0f + vk_absf(f) * 2500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, barb, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
