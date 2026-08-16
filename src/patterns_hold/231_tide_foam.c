/* pattern_231 — TIDE FOAM (field): sea foam lace sliding over dark sand —
 * bubbles packed into a net whose cells stretch and drift with the wash.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_231(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = 12.0f;
    const float wash = 0.35f * vk_sin(t * 0.0025f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f + wash;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells + 0.3f * vk_sin(v * 0.5f + t * 0.002f);
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float d1 = 9.0f, d2 = 9.0f;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float sx = cx + 0.5f + 0.4f * vk_sin(vk_h2(cx, cy, seed) * VK_TAU + t * 0.003f);
                float sy = cy + 0.5f + 0.4f * vk_cos(vk_h2(cx, cy, seed ^ 3u) * VK_TAU + t * 0.0025f);
                float dx = u - sx, dy = v - sy, d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) d2 = d;
            }
            float e = d2 - d1;
            float lace = vk_sstep(0.30f, 0.04f, e);          /* thick bubble walls */
            /* foam only where the wash reaches: a wandering front */
            float front = vk_noise2(u * 0.25f, t * 0.0015f, seed ^ 9u) * 4.0f + 2.5f + wash * 3.0f;
            float reach = vk_sstep(front + 1.5f, front - 1.5f, v - wash);
            float thin = vk_sstep(0.0f, 0.4f, d1);
            float m = lace * (0.6f + 0.4f * reach) * (0.7f + 0.3f * thin);
            float ci = base + e * 2500.0f + v * 150.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, thin, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
