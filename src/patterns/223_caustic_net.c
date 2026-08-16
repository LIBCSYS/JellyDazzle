/* pattern_223 — CAUSTIC NET (field): sunlight refracted through a rippled
 * surface onto the pool floor — a bright wandering net of caustic lines with
 * dark cells between.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_223(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.003f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 4.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 6.0f;
            /* caustic = product of two folded noise fields (thin bright ridges) */
            float n1 = vk_noise3(u, v, tz, seed), n2 = vk_noise3(u * 1.3f + 4.0f, v * 1.3f, tz * 1.2f + 2.0f, seed ^ 3u);
            float r1 = 1.0f - vk_absf(n1 - 0.5f) * 4.0f, r2 = 1.0f - vk_absf(n2 - 0.5f) * 4.0f;
            r1 = r1 < 0.0f ? 0.0f : r1; r2 = r2 < 0.0f ? 0.0f : r2;
            float c = r1 * 0.6f + r2 * 0.6f;
            /* wide soft glow plus sharp core */
            float m = vk_sstep(0.35f, 0.95f, c) * 0.85f + vk_sstep(0.9f, 1.15f, c) * 0.15f;
            float n3 = vk_noise3(u * 0.4f, v * 0.4f, tz * 0.5f, seed ^ 8u);
            float ci = base + n3 * 2600.0f + c * 700.0f + t * 0.5f;
            float cj = ci + 1500.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, r1, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
