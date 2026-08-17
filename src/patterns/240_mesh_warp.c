/* pattern_240 — MESH WARP (field): a soft grid mesh pushed and pulled by a
 * slow flow, lines thickening where they bunch, black cells between.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_240(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 6.0f, 9.0f);
    const float tz = t * 0.0015f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 2.0f;
            float wx = (vk_fbm3(u, v, tz, 2, seed) - 0.5f) * 0.5f;
            float wy = (vk_fbm3(u + 7.0f, v + 2.0f, tz, 2, seed ^ 3u) - 0.5f) * 0.5f;
            float a = (u + wx) * n, b = (v + wy) * n;
            float fa = vk_absf(vk_fract(a) - 0.5f), fbb = vk_absf(vk_fract(b) - 0.5f);
            float thick = 0.19f + 0.08f * vk_sin((a + b) * 0.7f + t * 0.003f);
            float la = vk_sstep(thick, thick * 0.2f, fa), lb = vk_sstep(thick, thick * 0.2f, fbb);
            float m = la > lb ? la : lb;
            /* mesh nodes glow */
            float node = vk_sstep(0.30f, 0.15f, sqrtf(fa * fa + fbb * fbb));
            m = m > node ? m : node;
            m *= 0.85f + 0.15f * node;
            float ci = base + (wx + wy) * 3000.0f + floorf(a) * 120.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, la, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
