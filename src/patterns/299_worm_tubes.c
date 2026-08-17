/* pattern_299 — WORM TUBES (field): fat glossy tubes winding across the
 * frame in slow S-bends, cylinder-shaded with a highlight stripe, crossing
 * over one another; black between the tubes.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NT 6
void pattern_299(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float m = 0.0f, ci = base;
            for (int i = 0; i < NT; i++) {
                float h1 = vk_seedf(seed, i), h2 = vk_seedf(seed, i + 10), h3 = vk_seedf(seed, i + 20);
                int vert = i & 1;
                float p = vert ? v : u, q = vert ? u : v;
                float c = (0.1f + 0.8f * h1) * (vert ? 1.333f : 1.0f) + 0.15f * vk_sin(p * (2.0f + 3.0f * h2) + t * 0.0018f + h3 * VK_TAU) + 0.05f * vk_sin(p * 7.0f - t * 0.0025f + i);
                float rad = 0.055f + 0.03f * h3;
                float d = (q - c) / rad;
                if (vk_absf(d) > 1.0f) continue;
                float body = vk_sstep(1.0f, 0.85f, vk_absf(d));
                float shade = sqrtf(1.0f - d * d);                    /* cylinder */
                float hi = vk_sstep(0.25f, 0.0f, vk_absf(d + 0.35f)) * 0.5f;
                float ring = 0.85f + 0.15f * vk_sin(p * 60.0f + t * 0.004f);
                float val = body * (0.35f + 0.65f * shade + hi) * ring;
                if (val > m) { m = val; ci = base + i * 600.0f + (d + 1.0f) * 800.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, 0.5f + 0.5f * vk_sin(u * 3.0f + v * 2.0f + t * 0.002f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
