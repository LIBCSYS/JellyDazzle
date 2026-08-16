/* pattern_331 — MOSAIC WAVE (field): square tesserae laid along a rolling
 * wave, each tile lit or shadowed by the slope it sits on, dark grout
 * between tiles and black troughs; the wave travels slowly.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_331(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 14.0f, 20.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            /* wave height field */
            float ph = u * 3.5f + v * 1.5f - t * 0.004f;
            float hgt = 0.5f + 0.5f * vk_sin(ph) + 0.25f * vk_sin(u * 2.0f - v * 3.0f + t * 0.0025f);
            float slope = vk_cos(ph);
            /* tiles follow the wave: displace the grid by the wave */
            float a = (u + 0.05f * vk_sin(ph)) * n, b = (v + 0.06f * hgt) * n;
            int ia = (int)floorf(a), ib = (int)floorf(b);
            float fa = vk_fract(a), fbb = vk_fract(b);
            float tile = vk_sstep(0.0f, 0.12f, fa) * vk_sstep(1.0f, 0.88f, fa) * vk_sstep(0.0f, 0.12f, fbb) * vk_sstep(1.0f, 0.88f, fbb);
            float lit = 0.5f + 0.5f * slope;
            float m = tile * vk_sstep(0.05f, 0.55f, lit) * vk_sstep(0.0f, 0.3f, hgt) * (0.7f + 0.3f * vk_h2(ia, ib, seed));
            float ci = base + vk_h2(ia, ib, seed ^ 3u) * 700.0f + hgt * 2000.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, lit, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
