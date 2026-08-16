/* pattern_227 — WIND GRASS (field): a meadow of tall grass blades bending
 * as gusts roll across, sky black above the tips and shadow black at the
 * roots between blades.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_227(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float nb = vk_seedr(seed, 1, 11.0f, 17.0f);
    for (int y = 0; y < sh; y++) {
        float v = 1.0f - (float)y / (float)sh;      /* 0 = ground */
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            /* gust: horizontal displacement grows with height^2 */
            float gust = vk_noise2(u * 1.5f - t * 0.004f, t * 0.0007f, seed) - 0.3f;
            float bend = (0.25f * gust + 0.05f * vk_sin(u * 6.0f + t * 0.005f)) * v * v;
            float uu = (u - bend) * nb;
            int ib = (int)floorf(uu);
            float f = vk_fract(uu) - 0.5f;
            float hgt = 0.55f + 0.4f * vk_h2(ib, 0, seed);
            float wdt = 0.42f * (1.0f - v / hgt) + 0.03f;
            float m = vk_sstep(wdt, wdt * 0.3f, vk_absf(f)) * vk_sstep(hgt, hgt - 0.05f, v);
            /* second, sparser layer behind, dimmer */
            float uu2 = (u - bend * 0.7f + 0.013f) * nb * 0.6f;
            int ib2 = (int)floorf(uu2);
            float f2 = vk_fract(uu2) - 0.5f;
            float hgt2 = 0.7f + 0.3f * vk_h2(ib2, 1, seed);
            float wdt2 = 0.36f * (1.0f - v / hgt2) + 0.03f;
            float m2 = vk_sstep(wdt2, wdt2 * 0.3f, vk_absf(f2)) * vk_sstep(hgt2, hgt2 - 0.05f, v) * 0.7f;
            float shade = 0.65f + 0.35f * v;
            float ci = base + v * 2200.0f + vk_h2(ib, 2, seed) * 500.0f + t * 0.5f;
            uint32_t c1 = vk_pc2(pal, ci, ci + 1600.0f, f + 0.5f, m * shade);
            uint32_t c2 = vk_pc2(pal, ci + 900.0f, ci + 2400.0f, f2 + 0.5f, m2 * shade);
            vk_putp(row + x * 3, vk_max(c1, c2));
        }
    }
    vk_blit(&cv, fb, w, h);
}
