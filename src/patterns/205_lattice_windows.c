/* pattern_205 — LATTICE WINDOWS (field): a diamond lattice of soft bars,
 * each window opening and closing on its own slow clock so the layer beneath
 * breathes through.  4-fold. Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_205(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = vk_seedr(seed, 2, 5.0f, 8.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f * cells;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f * cells;
            float a = u + v, b = u - v;          /* diamond axes */
            float fa = vk_fract(a), fbb = vk_fract(b);
            int ia = (int)floorf(a), ib = (int)floorf(b);
            float da = vk_absf(fa - 0.5f), db = vk_absf(fbb - 0.5f);
            /* per-cell breathing radius */
            float ph = vk_h2(ia, ib, seed) * VK_TAU;
            float open = 0.34f + 0.16f * vk_sin(t * 0.006f + ph);
            float d = da > db ? da : db;         /* square in diamond space */
            float bar = vk_sstep(open - 0.10f, open + 0.04f, d);
            /* soft shading across bar */
            float glow = 0.6f + 0.4f * vk_sin((a + b) * 1.5f + t * 0.004f);
            float m = bar * glow;
            float ci = base + vk_h2(ia, ib, seed ^ 3u) * 1600.0f + d * 3000.0f + t * 0.8f;
            vk_putp(row + x * 3, vk_pc(pal, ci, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
