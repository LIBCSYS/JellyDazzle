/* pattern_245 — ORBIT RINGS (field): three families of concentric rings
 * drifting around three centres, interfering into moire arcs, black water
 * between the ridges.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_245(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float cx[3], cy[3];
    for (int i = 0; i < 3; i++) {
        cx[i] = 0.55f * vk_sin(t * 0.0005f * (1.0f + 0.2f * i) + vk_seedf(seed, i) * VK_TAU);
        cy[i] = 0.45f * vk_cos(t * 0.0004f * (1.0f + 0.3f * i) + vk_seedf(seed, i + 5) * VK_TAU);
    }
    const float k = vk_seedr(seed, 1, 8.0f, 12.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float m = 0.0f, ci = base;
            for (int i = 0; i < 3; i++) {
                float dx = u - cx[i], dy = v - cy[i];
                float r = sqrtf(dx * dx + dy * dy);
                float f = vk_fract(r * k * (1.0f + 0.05f * i) - t * 0.0012f);
                float ring = vk_sstep(0.08f, 0.24f, f) * vk_sstep(0.50f, 0.34f, f);
                ring *= vk_sstep(1.3f, 0.7f, r);
                if (ring > m) { m = ring; ci = base + i * 1300.0f + r * 1500.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(u * 3.0f + v * 2.0f + t * 0.002f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
