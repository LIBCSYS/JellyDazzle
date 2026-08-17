/* pattern_222 — RIPPLE POND (field): three slow drop-sources send rings
 * across dark water; the crests catch light in colour, the troughs are
 * black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_222(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float cx[3], cy[3];
    for (int i = 0; i < 3; i++) {
        cx[i] = 0.5f + 0.4f * vk_sin(t * 0.0006f * (1.0f + 0.3f * i) + vk_seedf(seed, i) * VK_TAU);
        cy[i] = 0.5f + 0.4f * vk_cos(t * 0.0005f * (1.0f + 0.4f * i) + vk_seedf(seed, i + 7) * VK_TAU);
    }
    const float k = vk_seedr(seed, 1, 28.0f, 40.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float s = 0.0f;
            for (int i = 0; i < 3; i++) {
                float dx = u - cx[i] * 1.333f, dy = v - cy[i];
                float r = sqrtf(dx * dx + dy * dy);
                s += vk_sin(r * k - t * 0.02f + i * 2.0f) / (1.0f + r * 1.5f);
            }
            /* s in about -2..2 ; crests lit */
            float hgt = s * 0.4f + 0.5f;
            float m = vk_sstep(0.45f, 0.75f, hgt);
            float glint = vk_sstep(0.75f, 0.95f, hgt);
            float ci = base + hgt * 2400.0f + (u + v) * 500.0f + t * 0.5f;
            float cj = ci + 1800.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, glint, m * (0.7f + 0.3f * glint)));
        }
    }
    vk_blit(&cv, fb, w, h);
}
