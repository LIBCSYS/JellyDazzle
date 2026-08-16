/* pattern_332 — STARBURST FIELD (field): eight-pointed starbursts on a
 * staggered grid, each a soft radiating star that pulses and turns, black
 * between them.  8-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_332(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 2.0f, 2.8f);
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
            float ph = vk_h2((int)(rx * 2.0f), (int)ry, seed) * VK_TAU;
            float ang = atan2f(dy, dx) + t * 0.0006f * (((int)rx + (int)ry) & 1 ? 1 : -1) + ph;
            float pulse = 0.85f + 0.15f * vk_sin(t * 0.003f + ph);
            /* star: 8 rays, radius modulated */
            float ray = 0.5f + 0.5f * vk_cos(ang * 8.0f);
            float sr = (0.30f + 0.40f * vk_sstep(0.3f, 1.0f, ray)) * pulse;
            float star = vk_sstep(sr, sr * 0.6f, r);
            float core = vk_sstep(0.22f, 0.06f, r) * pulse;
            float m = star * (0.45f + 0.55f * ray);
            m = m > core ? m : core;
            float ci = base + vk_h2((int)(rx * 2.0f), (int)ry, seed ^ 3u) * 1500.0f + r * 2200.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, ray, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
