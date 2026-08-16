/* pattern_202 — SILK VEIL (field): a hanging silk with slow vertical folds
 * that sway; the folded-away faces go to black so the layer beneath shows
 * through the pleats.  No symmetry.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_202(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float folds = vk_seedr(seed, 1, 5.0f, 9.0f);
    const float base = vk_base(pal, seed, 4000);
    const float drift = t * 1.3f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw;
            float warp = 0.9f * vk_sin(v * 5.2f + t * 0.0041f + u * 1.4f)
                       + 0.5f * vk_sin(v * 11.0f - t * 0.0027f + 2.0f)
                       + 0.35f * vk_sin(u * 9.0f + v * 3.0f + t * 0.0019f);
            float p = u * folds * VK_TAU + warp;
            float shade = 0.5f + 0.5f * vk_cos(p);
            /* fine satin sheen riding on the fold */
            float sheen = 0.5f + 0.5f * vk_cos(p * 3.0f + v * 4.0f - t * 0.006f);
            float m = vk_sstep(0.14f, 0.75f, shade) * (0.7f + 0.3f * sheen);
            float hem = vk_sstep(0.0f, 0.10f, v + 0.05f * vk_sin(u * 7.0f + t * 0.003f))
                      * vk_sstep(1.0f, 0.90f, v + 0.05f * vk_sin(u * 5.0f - t * 0.002f));
            m *= hem;
            float ci = base + shade * 1800.0f + v * 2600.0f + drift;
            float cj = ci + 6000.0f + u * 2000.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, sheen * 0.7f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
