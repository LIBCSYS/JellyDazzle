/* pattern_246 — SINEW STRANDS (field): thick bundles of fibres running
 * across the frame in slow S-curves, each bundle striated, black between
 * the bundles.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_246(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float nb = vk_seedr(seed, 1, 4.0f, 6.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw;
            /* bundle axis: v shifted by an S-curve in u that drifts */
            float sv = v + 0.12f * vk_sin(u * 4.0f + t * 0.002f) + 0.06f * vk_sin(u * 9.0f - t * 0.0015f) + 0.04f * vk_noise2(u * 3.0f, t * 0.001f, seed);
            float b = sv * nb;
            int ib = (int)floorf(b);
            float f = vk_fract(b) - 0.5f;
            float wdt = 0.30f + 0.10f * vk_sin(u * 6.0f + ib * 2.0f + t * 0.0025f);
            float bundle = vk_sstep(wdt, wdt * 0.6f, vk_absf(f));
            /* striations: fibres within the bundle */
            float fib = 0.55f + 0.45f * vk_sin(f * 60.0f + u * 30.0f * (0.7f + 0.3f * vk_h2(ib, 0, seed)) + t * 0.004f);
            float shade = 0.5f + 0.5f * (1.0f - vk_absf(f) / wdt);
            float m = bundle * (0.6f + 0.4f * fib) * (0.6f + 0.4f * shade);
            float ci = base + vk_h2(ib, 1, seed) * 1800.0f + shade * 900.0f + u * 500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, fib, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
