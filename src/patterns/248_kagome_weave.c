/* pattern_248 — KAGOME WEAVE (field): bamboo strips woven in three
 * directions, the kagome lattice of triangles and hexagons, hex holes
 * black; the mat flexes.  6-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_248(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 5.0f, 8.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            u += 0.05f * vk_sin(v * 3.0f + t * 0.002f); v += 0.0f;
            float vv = v + 0.05f * vk_sin(u * 2.5f - t * 0.0017f);
            /* three strip families at 0, 60, 120 degrees */
            float p0 = u * n, p1 = (u * 0.5f + vv * 0.866f) * n, p2 = (u * 0.5f - vv * 0.866f) * n;
            float f0 = vk_absf(vk_fract(p0) - 0.5f), f1 = vk_absf(vk_fract(p1) - 0.5f), f2 = vk_absf(vk_fract(p2) - 0.5f);
            float wdt = 0.16f;
            float s0 = vk_sstep(wdt, wdt * 0.5f, f0), s1 = vk_sstep(wdt, wdt * 0.5f, f1), s2 = vk_sstep(wdt, wdt * 0.5f, f2);
            /* over/under shading: which family is on top cycles by cell */
            int c = ((int)floorf(p0) + (int)floorf(p1) * 2 + (int)floorf(p2) * 3) % 3;
            if (c < 0) c += 3;
            float sh0 = 0.6f + 0.4f * vk_sin(f0 / wdt * 3.0f), sh1 = 0.6f + 0.4f * vk_sin(f1 / wdt * 3.0f), sh2 = 0.6f + 0.4f * vk_sin(f2 / wdt * 3.0f);
            float m, ci;
            float top = c == 0 ? s0 * sh0 : c == 1 ? s1 * sh1 : s2 * sh2;
            float rest = s0 * sh0 > s1 * sh1 ? s0 * sh0 : s1 * sh1; rest = rest > s2 * sh2 ? rest : s2 * sh2;
            m = top > 0.05f ? top : rest * 0.75f;
            ci = base + (top > 0.05f ? c : (s0 > s1 ? (s0 > s2 ? 0 : 2) : (s1 > s2 ? 1 : 2))) * 1200.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, 0.5f + 0.5f * vk_sin((u + vv) * 4.0f + t * 0.002f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
