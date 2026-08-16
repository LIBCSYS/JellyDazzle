/* pattern_302 — CIRRUS STREAKS (field): high cirrus drawn out by wind into
 * long diagonal streaks, wisps combed and feathered, deep black sky showing
 * between them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_302(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float ang = vk_seedr(seed, 1, 0.3f, 0.8f);
    const float ca = vk_cos(ang), sa = vk_sin(ang);
    const float tz = t * 0.001f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float a = u * ca + v * sa + t * 0.0006f, b = -u * sa + v * ca;
            /* streaks: noise stretched along a (low freq), fine across b */
            float n = vk_fbm3(a * 1.2f, b * 9.0f, tz, 3, seed);
            float n2 = vk_noise3(a * 3.0f, b * 30.0f, tz * 1.5f, seed ^ 7u);
            float dens = n * 0.7f + n2 * 0.3f;
            float m = vk_sstep(0.45f, 0.72f, dens);
            /* wispy tails: fade along a */
            m *= 0.6f + 0.4f * vk_noise2(a * 4.0f, b * 2.0f, seed ^ 3u);
            float ci = base + b * 2500.0f + n * 800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, n2, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
