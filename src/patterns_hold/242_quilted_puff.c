/* pattern_242 — QUILTED PUFF (field): a puffy quilt — soft domed squares
 * on a diagonal grid, dark stitched seams between, panels breathing in and
 * out of the light.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_242(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 4.0f, 6.0f);
    const float ang = vk_seedr(seed, 2, 0.3f, 0.9f);
    const float ca = vk_cos(ang), sa = vk_sin(ang);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float a = (u * ca - v * sa) * n, b = (u * sa + v * ca) * n;
            a += 0.08f * vk_sin(b * 1.5f + t * 0.002f); b += 0.08f * vk_sin(a * 1.2f - t * 0.0016f);
            int ia = (int)floorf(a), ib = (int)floorf(b);
            float fa = vk_fract(a) - 0.5f, fbb = vk_fract(b) - 0.5f;
            /* dome: superellipse falloff */
            float d = powf(vk_absf(fa) * 2.0f, 3.0f) + powf(vk_absf(fbb) * 2.0f, 3.0f);
            float dome = 1.0f - vk_sstep(0.5f, 1.0f, d);
            float lit = 0.5f + 0.5f * vk_sin(t * 0.003f + vk_h2(ia, ib, seed) * VK_TAU);
            float shine = 0.6f + 0.4f * (1.0f - d) * lit;
            float m = dome * vk_sstep(0.15f, 0.55f, lit) * shine;
            /* stitch highlights along seams */
            float seam = vk_sstep(0.03f, 0.0f, vk_absf(vk_absf(fa) - 0.5f) < vk_absf(vk_absf(fbb) - 0.5f) ? vk_absf(vk_absf(fa) - 0.5f) : vk_absf(vk_absf(fbb) - 0.5f));
            m = m > seam * 0.5f ? m : seam * 0.5f;
            float ci = base + vk_h2(ia, ib, seed ^ 5u) * 2000.0f + d * 800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, lit, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
