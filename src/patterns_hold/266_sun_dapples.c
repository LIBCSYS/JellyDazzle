/* pattern_266 — SUN DAPPLES (field): sunlight through a swaying canopy —
 * soft overlapping discs of light on the ground, moving with the leaves,
 * the shade between them black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_266(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.003f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 3.0f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 4.0f;
            /* canopy gaps: two noise fields multiplied = discs of light */
            float sway = 0.15f * vk_sin(tz + v * 0.5f);
            float n1 = vk_noise3(u + sway, v, tz * 0.7f, seed);
            float n2 = vk_noise3(u * 1.7f - sway, v * 1.7f + 3.0f, tz * 0.9f + 5.0f, seed ^ 4u);
            float n3 = vk_noise3(u * 3.1f, v * 3.1f - sway, tz * 1.1f + 9.0f, seed ^ 8u);
            float light = vk_sstep(0.45f, 0.75f, n1 * 0.5f + n2 * 0.5f);
            float fleck = vk_sstep(0.55f, 0.85f, n3) * 0.6f;
            float m = light * (0.7f + 0.3f * n3);
            m = m > fleck * light ? m : fleck * light;
            m = m > fleck * 0.5f ? m : fleck * 0.5f;
            float ci = base + n1 * 2200.0f + n2 * 900.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, n3, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
