/* pattern_210 — OIL SLICK (field): thin-film interference on dark water —
 * bands of colour follow the film thickness, and where the film breaks the
 * water is black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_210(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.002f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 2.2f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 3.0f;
            float q = vk_fbm3(u, v, tz, 2, seed);
            float thick = vk_fbm3(u + 2.0f * q, v - 1.5f * q, tz * 0.7f + 4.0f, 3, seed ^ 9u);
            /* film present where thick enough; bands from thickness */
            float m = vk_sstep(0.38f, 0.50f, thick);
            float band = thick * 9.0f + t * 0.003f;
            float fr = vk_fract(band);
            float ring = 0.65f + 0.35f * vk_sin(band * VK_TAU);
            float ci = base + fr * 3200.0f + q * 800.0f + t * 0.4f;
            float cj = ci + 1400.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, ring, m * ring));
        }
    }
    vk_blit(&cv, fb, w, h);
}
