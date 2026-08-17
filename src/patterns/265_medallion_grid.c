/* pattern_265 — MEDALLION GRID (field): a carpet of twelve-fold medallions
 * on a staggered grid, each medallion a ring of lobes around a glowing
 * boss, black field between them.  12-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_265(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 2.2f, 3.2f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f * n;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f * n;
            float ry = floorf(v);
            float ox = ((int)ry & 1) ? 0.5f : 0.0f;
            float rx = floorf(u - ox) + ox;
            float dx = u - rx - 0.5f, dy = v - ry - 0.5f;
            float r = sqrtf(dx * dx + dy * dy);
            float ang = atan2f(dy, dx) + t * 0.0006f * (((int)rx + (int)ry) & 1 ? 1.0f : -1.0f);
            float ph = vk_h2((int)(rx * 2.0f), (int)ry, seed) * VK_TAU;
            float breathe = 0.03f * vk_sin(t * 0.003f + ph);
            /* lobes */
            float lr = 0.44f + 0.06f * vk_cos(ang * 12.0f) + breathe;
            float outer = vk_sstep(lr, lr - 0.05f, r) * vk_sstep(0.24f, 0.29f, r);
            float inner = vk_sstep(0.24f, 0.20f, r) * (0.6f + 0.4f * vk_sstep(0.20f, 0.0f, r));
            float spokes = 0.6f + 0.4f * vk_sstep(0.3f, 0.9f, 0.5f + 0.5f * vk_cos(ang * 12.0f + 3.14159f));
            float m = outer * spokes;
            m = m > inner ? m : inner;
            float ci = base + vk_h2((int)(rx * 2.0f), (int)ry, seed ^ 7u) * 1500.0f + r * 2500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, 0.5f + 0.5f * vk_cos(ang * 12.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
