/* pattern_287 — CHECKER BREATH (field): a soft-edged checkerboard turned at
 * an angle, each square swelling into light and sinking back to black on
 * its own slow rhythm, so islands of squares drift through the field.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_287(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 5.0f, 8.0f);
    const float ang = vk_seedr(seed, 2, 0.15f, 0.6f) + 0.02f * vk_sin(t * 0.0008f);
    const float ca = vk_cos(ang), sa = vk_sin(ang);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float a = (u * ca - v * sa) * n, b = (u * sa + v * ca) * n;
            int ia = (int)floorf(a), ib = (int)floorf(b);
            float fa = vk_fract(a) - 0.5f, fbb = vk_fract(b) - 0.5f;
            float d = vk_absf(fa) > vk_absf(fbb) ? vk_absf(fa) : vk_absf(fbb);
            /* breathing: a travelling wave plus per-cell phase */
            float ph = vk_h2(ia, ib, seed) * 2.0f;
            float breath = 0.5f + 0.5f * vk_sin(t * 0.004f + ph + (a + b) * 0.35f);
            float on = ((ia + ib) & 1) ? breath : 1.0f - breath;
            float size = 0.48f * vk_sstep(0.15f, 0.9f, on);
            float m = vk_sstep(size, size * 0.6f, d) * (0.5f + 0.5f * on);
            float ci = base + vk_h2(ia, ib, seed ^ 3u) * 900.0f + on * 1800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, d * 2.0f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
