/* pattern_319 — LATTICE BLOOM (field): a square lattice of flowers, each
 * opening and closing on its own clock — petals unfolding from a bud to a
 * full four-fold bloom and back; black between.  4-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_319(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 3.5f, 5.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f * n;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f * n;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float dx = vk_fract(u) - 0.5f, dy = vk_fract(v) - 0.5f;
            float r = sqrtf(dx * dx + dy * dy);
            float ang = atan2f(dy, dx) + t * 0.0005f * (((iu + iv) & 1) ? 1.0f : -1.0f);
            float ph = vk_h2(iu, iv, seed) * VK_TAU;
            float open = 0.5f + 0.5f * vk_sin(t * 0.0025f + ph);
            /* petals: 4 (or 8 when open) lobes */
            float lobes = 4.0f + 4.0f * vk_sstep(0.5f, 0.9f, open);
            float pr = (0.36f + 0.34f * open) * (0.72f + 0.28f * vk_cos(ang * lobes));
            float petal = vk_sstep(pr, pr - 0.08f, r);
            float vein = 0.75f + 0.25f * vk_sstep(0.7f, 1.0f, 0.5f + 0.5f * vk_cos(ang * lobes));
            float centre = vk_sstep(0.09f, 0.03f, r) * 0.9f;
            float m = petal * vein * (0.65f + 0.35f * open);
            m = m > centre ? m : centre;
            float ci = base + vk_h2(iu, iv, seed ^ 3u) * 1600.0f + r * 1800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, open, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
