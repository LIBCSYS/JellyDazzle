/* pattern_335 — AURORA DOME (field): a corona of aurora rays converging on
 * the zenith overhead — rays streaming inward from the horizon, brightest
 * near the top, curtains breathing; black sky between rays.  Mirror
 * symmetry.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_335(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.003f;
    const float zx = 0.0f, zy = 0.35f + 0.05f * vk_sin(t * 0.0007f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = vk_absf((float)x / (float)sw - 0.5f) * 2.0f;
            float dx = u - zx, dy = v - zy;
            float r = sqrtf(dx * dx + dy * dy) + 1e-3f;
            float ang = atan2f(dx, dy);
            /* rays: noise in angle, drifting; brightness falls with r but rays reach far */
            float ray = vk_noise2(ang * 12.0f + tz * 0.3f, tz * 0.6f, seed) * 0.6f + vk_noise2(ang * 40.0f - tz * 0.5f, tz, seed ^ 5u) * 0.4f;
            float band = vk_noise2(ang * 3.0f, r * 2.0f - tz * 0.4f, seed ^ 9u);
            float m = vk_sstep(0.35f, 0.65f, ray) * vk_sstep(0.3f, 0.6f, band + 0.2f) * vk_sstep(0.05f, 0.25f, r) * vk_sstep(1.5f, 0.6f, r);
            m *= 0.6f + 0.4f * ray;
            float ci = base + r * 2800.0f + ray * 900.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, band, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
