/* pattern_233 — CRINKLED FOIL (field): crumpled metal foil catching a moving
 * light — bright facets, black creases and shadowed facets, slowly flexing.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_233(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = vk_seedr(seed, 1, 7.0f, 11.0f);
    const float lx = vk_cos(t * 0.0012f), ly = vk_sin(t * 0.0009f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float d1 = 9.0f, d2 = 9.0f, nx = 0, ny = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float sx = cx + 0.1f + 0.8f * vk_h2(cx, cy, seed);
                float sy = cy + 0.1f + 0.8f * vk_h2(cx, cy, seed ^ 1u);
                float dx = u - sx, dy = v - sy, d = dx * dx + dy * dy;
                if (d < d1) { d2 = d1; d1 = d; nx = vk_h2(cx, cy, seed ^ 2u) - 0.5f; ny = vk_h2(cx, cy, seed ^ 3u) - 0.5f; }
                else if (d < d2) d2 = d;
            }
            /* facet normal fixed per cell; light direction rotates slowly */
            float lit = 0.5f + (nx * lx + ny * ly) * 1.6f + 0.15f * vk_sin(t * 0.004f + nx * 20.0f);
            float crease = vk_sstep(0.0f, 0.06f, sqrtf(d2) - sqrtf(d1));
            float m = crease * vk_sstep(0.15f, 0.65f, lit);
            float ci = base + lit * 2400.0f + (nx + ny) * 800.0f + t * 0.4f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, ny + 0.5f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
