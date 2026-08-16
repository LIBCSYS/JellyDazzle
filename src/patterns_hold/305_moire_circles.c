/* pattern_305 — MOIRE CIRCLES (field): two sets of concentric circles with
 * centres drifting apart and together, their moire making broad hyperbolic
 * fringes — bright fringes on black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_305(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float k = vk_seedr(seed, 1, 40.0f, 60.0f);
    const float sep = 0.25f + 0.15f * vk_sin(t * 0.0008f);
    const float ax = -sep, ay = 0.05f * vk_sin(t * 0.0006f), bx = sep, by = -0.05f * vk_sin(t * 0.0007f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float d1 = sqrtf((u - ax) * (u - ax) + (v - ay) * (v - ay));
            float d2 = sqrtf((u - bx) * (u - bx) + (v - by) * (v - by));
            float beat = 0.5f + 0.5f * vk_cos((d1 - d2) * k - t * 0.006f);
            float fine = 0.5f + 0.5f * vk_cos((d1 + d2) * k * 0.5f);
            float m = vk_sstep(0.3f, 0.75f, beat) * (0.6f + 0.4f * fine);
            float ci = base + beat * 2000.0f + (d1 + d2) * 900.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, fine, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
