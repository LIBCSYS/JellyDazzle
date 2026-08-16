/* pattern_230 — NEBULA WISPS (field): filaments of glowing gas drift and
 * fold, brighter knots along them, deep space black between.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_230(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.002f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 2.4f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 3.2f;
            float q = vk_fbm3(u * 0.8f, v * 0.8f, tz, 3, seed);
            float n = vk_fbm3(u + q * 2.5f, v - q * 1.5f, tz * 0.8f, 4, seed ^ 7u);
            /* ridged: filaments where n crosses 0.5 */
            float fil = 1.0f - vk_absf(n - 0.5f) * 5.0f;
            fil = fil < 0.0f ? 0.0f : fil;
            float body = vk_sstep(0.55f, 0.80f, n) * 0.5f;
            float m = fil * fil * 1.0f + body * 1.2f;
            m = m > 1.0f ? 1.0f : m;
            m *= vk_sstep(0.40f, 0.60f, q);
            float ci = base + q * 3000.0f + n * 900.0f + t * 0.4f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1800.0f, fil, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
