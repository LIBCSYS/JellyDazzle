/* pattern_317 — DUNE SEA (field): an aerial view of a dune sea — long
 * sinuous crests lit on one flank with deep black shadow on the other,
 * broken by dark interdune flats, the whole erg creeping downwind.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_317(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.0006f;
    const float k = vk_seedr(seed, 1, 5.0f, 8.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 2.0f;
            float warp = vk_fbm3(u * 1.2f, v * 1.2f, tz, 3, seed);
            float p = (v + 0.3f * u) * k + warp * 4.0f - t * 0.002f;
            float f = vk_fract(p);
            /* asymmetric crest: long lit windward slope, short black lee */
            float lit = f < 0.7f ? f / 0.7f : 1.0f - (f - 0.7f) / 0.3f;
            float crest = vk_sstep(0.2f, 0.9f, lit);
            /* interdune flats: black where a big noise is low */
            float flat = vk_sstep(0.30f, 0.44f, vk_fbm3(u * 0.8f + 5.0f, v * 0.8f, tz * 0.7f, 2, seed ^ 9u));
            float ripple = 0.85f + 0.15f * vk_sin(u * 40.0f + v * 10.0f + warp * 12.0f);
            float m = crest * flat * ripple;
            float ci = base + lit * 1500.0f + warp * 1500.0f + t * 0.4f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, ripple, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
