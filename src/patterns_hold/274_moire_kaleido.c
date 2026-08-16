/* pattern_274 — MOIRE KALEIDO (field): ring-moire folded through a six-fold
 * kaleidoscope — two ring sources drift inside the wedge, their beat pattern
 * repeating around the centre, black where the rings cancel.  6-fold.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_274(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float k = vk_seedr(seed, 1, 22.0f, 30.0f);
    const float ax = 0.35f + 0.15f * vk_sin(t * 0.0011f), ay = 0.10f * vk_sin(t * 0.0009f);
    const float bx = 0.20f + 0.15f * vk_sin(t * 0.0007f + 2.0f), by = 0.25f + 0.10f * vk_cos(t * 0.0013f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u) + t * 0.0003f;
            /* fold into a 30-degree wedge */
            float wa = VK_TAU / 12.0f;
            float fa = fmodf(ang, 2.0f * wa); if (fa < 0.0f) fa += 2.0f * wa;
            if (fa > wa) fa = 2.0f * wa - fa;
            float px = r * vk_cos(fa), py = r * vk_sin(fa);
            float d1 = sqrtf((px - ax) * (px - ax) + (py - ay) * (py - ay));
            float d2 = sqrtf((px - bx) * (px - bx) + (py - by) * (py - by));
            float s1 = 0.5f + 0.5f * vk_sin(d1 * k - t * 0.01f), s2 = 0.5f + 0.5f * vk_sin(d2 * k * 1.06f + t * 0.008f);
            float beat = 0.5f + 0.5f * vk_cos(d1 * k - d2 * k * 1.06f - t * 0.018f);
            float m = vk_sstep(0.25f, 0.65f, beat) * (0.5f + 0.5f * vk_sstep(0.3f, 0.8f, s1 * 0.5f + s2 * 0.5f)) * vk_sstep(1.3f, 0.9f, r);
            float ci = base + beat * 2400.0f + r * 700.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, s1, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
