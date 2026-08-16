/* pattern_261 — GAUZE LAYERS (field): three sheer gauzes hung one behind
 * another, each an open weave drifting at its own pace; where the weaves
 * align the light comes through in soft blooms, elsewhere the cloth reads as
 * threads with black between.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_261(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float acc = 0.0f, hue = 0.0f;
            for (int L = 0; L < 3; L++) {
                float k = 14.0f + 3.0f * L;
                float ox = t * 0.0003f * (L - 1) + 0.03f * vk_sin(v * 4.0f + t * 0.002f + L);
                float oy = t * 0.0002f * (L & 1 ? 1 : -1) + 0.03f * vk_sin(u * 3.0f - t * 0.0015f + L * 2.0f);
                float a = (u + ox) * k, b = (v + oy) * k;
                float th = 0.5f + 0.5f * vk_cos(a * VK_TAU);
                float tv = 0.5f + 0.5f * vk_cos(b * VK_TAU);
                float thread = th > tv ? th : tv;
                acc += vk_sstep(0.5f, 0.95f, thread) * 0.4f;
                hue += th * tv * (L + 1) * 400.0f;
            }
            /* blooms where all three overlap; drop the faint single-thread areas */
            float m = vk_sstep(0.5f, 1.05f, acc);
            float glow = vk_noise2(u * 3.0f + t * 0.001f, v * 3.0f, seed);
            m *= 0.6f + 0.4f * glow;
            float ci = base + hue + glow * 1600.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, glow, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
