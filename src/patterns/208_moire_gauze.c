/* pattern_208 — MOIRE GAUZE (field): two soft gratings laid over each other
 * at a slowly turning angle; the beat between them makes broad moire fringes,
 * and where they cancel the gauze goes black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_208(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float k = vk_seedr(seed, 1, 30.0f, 44.0f);
    const float a1 = t * 0.0005f, a2 = 0.12f + 0.06f * vk_sin(t * 0.0009f) - t * 0.0003f;
    const float c1 = vk_cos(a1), s1 = vk_sin(a1), c2 = vk_cos(a2), s2 = vk_sin(a2);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float bend = 0.12f * vk_sin(u * 2.0f + v * 3.0f + t * 0.002f);
            float p1 = (u * c1 - v * s1 + bend) * k;
            float p2 = (u * c2 - v * s2 - bend) * (k * 1.04f);
            float g1 = 0.5f + 0.5f * vk_sin(p1), g2 = 0.5f + 0.5f * vk_sin(p2);
            float env = 0.5f + 0.5f * vk_cos(p1 - p2);       /* the beat */
            float g = g1 * 0.5f + g2 * 0.5f;
            float m = vk_sstep(0.20f, 0.60f, env) * (0.45f + 0.55f * vk_sstep(0.3f, 0.8f, g));
            float ci = base + env * 2600.0f + (u - v) * 500.0f + t * 0.8f;
            float cj = ci + 1500.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, g, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
