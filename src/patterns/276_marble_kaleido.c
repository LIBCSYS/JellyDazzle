/* pattern_276 — MARBLE KALEIDO (field): marbled veins folded through an
 * eight-fold mirror, veins swirling toward the centre, the ground between
 * veins black.  8-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_276(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.0015f;
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u) + t * 0.0002f;
            float wa = VK_TAU / 16.0f;
            float fa = fmodf(ang, 2.0f * wa); if (fa < 0.0f) fa += 2.0f * wa;
            if (fa > wa) fa = 2.0f * wa - fa;
            float px = r * vk_cos(fa) * 3.0f, py = r * vk_sin(fa) * 3.0f;
            float n = vk_fbm3(px, py, tz, 4, seed);
            float p = (px * 0.8f + py * 1.6f) * 2.5f + n * 7.0f + t * 0.002f;
            float s = 0.5f + 0.5f * vk_sin(p);
            float m = vk_sstep(0.3f, 0.6f, s) * vk_sstep(1.0f, 0.85f, s) * vk_sstep(1.35f, 1.0f, r);
            float sub = 0.5f + 0.5f * vk_sin(p * 3.0f + n * 4.0f);
            float ci = base + n * 2400.0f + s * 900.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, sub, m * (0.8f + 0.2f * sub)));
        }
    }
    vk_blit(&cv, fb, w, h);
}
