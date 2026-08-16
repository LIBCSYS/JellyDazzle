/* pattern_321 — RING TUNNEL (field): concentric rings receding into a
 * tunnel whose centre wanders, rings soft and thick with black gaps, spokes
 * of shadow turning through them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_321(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cx = 0.25f * vk_sin(t * 0.0007f), cy = 0.18f * vk_cos(t * 0.0005f);
    const int spokes = 6 + 2 * (int)(vk_seedf(seed, 1) * 3.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f - cy;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f - cx;
            float r = sqrtf(u * u + v * v) + 0.02f;
            float ang = atan2f(v, u);
            float depth = 0.25f / r + t * 0.003f;              /* rings recede */
            float f = vk_fract(depth);
            float ring = vk_sstep(0.10f, 0.30f, f) * vk_sstep(0.62f, 0.42f, f);
            float sp = 0.5f + 0.5f * vk_sin(ang * spokes + t * 0.0015f + depth * 0.5f);
            float m = ring * (0.15f + 0.85f * vk_sstep(0.3f, 0.7f, sp)) * vk_sstep(0.05f, 0.15f, r) * vk_sstep(1.5f, 1.0f, r);
            float ci = base + floorf(depth) * 500.0f + f * 800.0f + sp * 600.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, sp, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
