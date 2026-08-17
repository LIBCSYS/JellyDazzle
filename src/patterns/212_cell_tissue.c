/* pattern_212 — CELL TISSUE (field): living tissue — soft cells packed
 * edge to edge, each glowing from its nucleus, dark walls between them and
 * some cells dormant (dark).  Voronoi with jittered breathing sites.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_212(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = vk_seedr(seed, 1, 5.0f, 8.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float d1 = 9.0f, d2 = 9.0f, id = 0.0f;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float ph = vk_h2(cx, cy, seed ^ 5u) * VK_TAU;
                float sx = cx + 0.5f + 0.32f * vk_sin(t * 0.0037f + ph);
                float sy = cy + 0.5f + 0.32f * vk_cos(t * 0.0029f + ph * 1.7f);
                float dx = u - sx, dy = v - sy, d = dx * dx + dy * dy;
                if (d < d1) { d2 = d1; d1 = d; id = vk_h2(cx, cy, seed); }
                else if (d < d2) d2 = d;
            }
            float e = sqrtf(d2) - sqrtf(d1);            /* distance to wall */
            float wall = vk_sstep(0.03f, 0.16f, e);
            float alive = vk_sstep(0.25f, 0.45f, id + 0.15f * vk_sin(t * 0.004f + id * 20.0f));
            float glow = 1.0f - vk_sstep(0.0f, 0.55f, sqrtf(d1));
            float m = wall * alive * (0.35f + 0.65f * glow);
            float ci = base + id * 2600.0f + glow * 900.0f + t * 0.6f;
            float cj = ci + 1600.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, glow, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
