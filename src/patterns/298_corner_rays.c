/* pattern_298 — CORNER RAYS (field): light streaming from one corner in
 * soft diverging rays, as through blinds — bright shafts and black gaps that
 * slowly sweep and breathe.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_298(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const int corner = (int)(seed & 3u);
    const float ox = (corner & 1) ? 1.45f : -0.12f, oy = (corner & 2) ? 1.12f : -0.12f;
    const float nr = vk_seedr(seed, 1, 14.0f, 22.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float dx = u - ox, dy = v - oy;
            float r = sqrtf(dx * dx + dy * dy);
            float ang = atan2f(dy, dx);
            float sweep = 0.06f * vk_sin(t * 0.0015f) + 0.02f * vk_sin(r * 4.0f + t * 0.003f);
            float a = (ang + sweep) * nr / VK_TAU;
            float f = vk_fract(a);
            float wdt = 0.5f + 0.2f * vk_sin(floorf(a) * 1.7f + t * 0.002f);
            float ray = vk_sstep(0.0f, 0.15f, f) * vk_sstep(wdt + 0.15f, wdt, f);
            /* dust in the beam: soft noise */
            float dust = 0.6f + 0.4f * vk_noise2(r * 6.0f - t * 0.003f, ang * 20.0f, seed);
            float m = ray * dust * vk_sstep(0.05f, 0.4f, r) * vk_sstep(2.0f, 0.9f, r);
            float ci = base + r * 1500.0f + f * 700.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, dust, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
