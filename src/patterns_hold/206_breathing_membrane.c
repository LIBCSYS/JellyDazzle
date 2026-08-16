/* pattern_206 — BREATHING MEMBRANE (field): a stretched cell membrane, its
 * surface heaving with slow bulges; the thin, stretched parts go transparent
 * so the ground glows through the pores.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_206(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.004f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 3.0f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 4.0f;
            float n = vk_fbm3(u, v, tz, 3, seed);
            float n2 = vk_noise3(u * 2.3f + 5.0f, v * 2.3f, tz * 1.4f + 3.0f, seed ^ 11u);
            float thick = n * 0.7f + n2 * 0.3f;
            float m = vk_sstep(0.36f, 0.60f, thick);
            /* iridescent thickness bands, like a soap film */
            float band = vk_fract(thick * 4.0f + t * 0.0015f);
            float ci = base + band * 2600.0f + thick * 2500.0f + t * 0.6f;
            float cj = ci + 5500.0f;
            float lit = 0.75f + 0.25f * vk_sin(thick * 22.0f + t * 0.01f);
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, band, m * lit));
        }
    }
    vk_blit(&cv, fb, w, h);
}
