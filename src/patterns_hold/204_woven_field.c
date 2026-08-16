/* pattern_204 — WOVEN FIELD (field): two families of wavy bands cross and
 * weave over/under; the gaps of the weave are black windows.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_204(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 3, 5.0f, 8.0f);
    const float ang = vk_seedr(seed, 4, 0.0f, VK_TAU);
    const float ca = vk_cos(ang), sa = vk_sin(ang);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float a = u * ca - v * sa, b = u * sa + v * ca;
            float wa = a * n + 0.5f * vk_sin(b * 3.0f + t * 0.0031f);
            float wb = b * n + 0.5f * vk_sin(a * 2.6f - t * 0.0027f);
            float fa = vk_fract(wa), fbb = vk_fract(wb);
            /* band profile: bright ridge, soft edge, black gap */
            float ba = vk_sstep(0.14f, 0.34f, fa) * vk_sstep(0.68f, 0.48f, fa);
            float bb = vk_sstep(0.14f, 0.34f, fbb) * vk_sstep(0.68f, 0.48f, fbb);
            /* over/under: whichever family is on top depends on the cell */
            int cx = (int)floorf(wa), cy = (int)floorf(wb);
            int over = (cx + cy) & 1;
            float top = over ? ba : bb, under = over ? bb : ba;
            float shade = top > 0.02f ? top : under * 0.55f;
            float m = shade;
            float ci = base + (over ? 0.0f : 2400.0f) + (top > 0.02f ? fa : fbb) * 900.0f
                     + t * 1.1f + (a + b) * 500.0f;
            vk_putp(row + x * 3, vk_pc(pal, ci, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
