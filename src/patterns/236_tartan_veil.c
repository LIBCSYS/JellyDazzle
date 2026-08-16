/* pattern_236 — TARTAN VEIL (field): a sheer plaid — translucent bands in
 * two directions, opaque colour where they cross, black between; the veil
 * ripples.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_236(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 3.0f, 5.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float a = u + 0.05f * vk_sin(v * 4.0f + t * 0.003f), b = v + 0.05f * vk_sin(u * 3.0f - t * 0.0022f);
            /* sett: wide band + two narrow pinstripes per period */
            float fa = vk_fract(a * n), fbb = vk_fract(b * n);
            float wa = vk_sstep(0.05f, 0.13f, fa) * vk_sstep(0.38f, 0.30f, fa) + 0.7f * vk_sstep(0.70f, 0.72f, fa) * vk_sstep(0.78f, 0.76f, fa) + 0.7f * vk_sstep(0.85f, 0.87f, fa) * vk_sstep(0.93f, 0.91f, fa);
            float wb = vk_sstep(0.05f, 0.13f, fbb) * vk_sstep(0.38f, 0.30f, fbb) + 0.7f * vk_sstep(0.70f, 0.72f, fbb) * vk_sstep(0.78f, 0.76f, fbb) + 0.7f * vk_sstep(0.85f, 0.87f, fbb) * vk_sstep(0.93f, 0.91f, fbb);
            float m = (wa > wb ? wa : wb) * 0.6f + wa * wb * 0.4f;
            m = m > 1.0f ? 1.0f : m;
            float weave = 0.85f + 0.15f * vk_sin((a + b) * 90.0f);
            float ci = base + wa * 1500.0f + wb * 2600.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, wa, m * weave));
        }
    }
    vk_blit(&cv, fb, w, h);
}
