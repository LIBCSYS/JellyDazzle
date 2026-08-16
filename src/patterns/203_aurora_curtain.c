/* pattern_203 — AURORA CURTAIN (field): tall rays hang from a wavering upper
 * edge, brightness rippling along the curtain in bands of colour; the sky
 * between rays and below the hem stays black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_203(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.0035f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw;
            /* the curtain folds in depth: x is warped by height */
            float uw = u + 0.06f * vk_sin(v * 4.0f + t * 0.003f) + 0.03f * vk_sin(v * 9.0f - t * 0.002f + u * 3.0f);
            float ray = vk_fbm2(uw * 6.0f + tz * 0.6f, tz, 3, seed);
            float ray2 = vk_noise2(uw * 40.0f - tz * 0.4f, tz * 1.3f, seed ^ 77u);
            float top = 0.04f + 0.14f * vk_noise2(u * 3.0f + tz * 0.5f, 3.0f, seed ^ 5u);
            float bot = 0.70f + 0.28f * vk_noise2(u * 2.2f - tz * 0.3f, 9.0f, seed ^ 9u);
            float vert = vk_sstep(top - 0.06f, top + 0.10f, v) * vk_sstep(bot + 0.12f, bot - 0.30f, v);
            float r = ray * 0.55f + ray2 * 0.45f;
            float m = vert * vk_sstep(0.30f, 0.55f, r) * (0.7f + 0.3f * ray2);
            float ci = base + v * 3000.0f + ray * 900.0f + t * 0.9f;
            float cj = ci + 1800.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, ray2, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
