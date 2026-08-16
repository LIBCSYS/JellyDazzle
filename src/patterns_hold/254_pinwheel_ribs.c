/* pattern_254 — PINWHEEL RIBS (field): five curved vanes of a slow
 * pinwheel, each vane ribbed across its width, black wedges between the
 * vanes.  5-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_254(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float vanes = 5.0f;
    const float curl = vk_seedr(seed, 1, 1.5f, 3.0f);
    const float rot = t * 0.0006f;
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u) + rot + r * curl;
            float f = vk_fract(ang * vanes / VK_TAU);
            /* vane occupies 0.6 of each sector, soft edges */
            float vane = vk_sstep(0.05f, 0.20f, f) * vk_sstep(0.70f, 0.55f, f);
            /* ribs across the vane (radial ripples) */
            float rib = 0.6f + 0.4f * vk_sin(r * 40.0f - t * 0.006f);
            float taper = vk_sstep(1.3f, 0.7f, r) * vk_sstep(0.0f, 0.12f, r);
            float m = vane * (0.6f + 0.4f * rib) * taper;
            float ci = base + f * 1500.0f + r * 2200.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, rib, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
