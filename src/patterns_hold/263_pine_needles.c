/* pattern_263 — PINE NEEDLES (field): clusters of pine needles bursting
 * from scattered buds, each fascicle a soft radial spray, needles swaying,
 * black between clusters.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_263(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = 3.5f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float m = 0.0f, ci = base;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float h1 = vk_h2(cx, cy, seed), h2 = vk_h2(cx, cy, seed ^ 3u);
                float sx = cx + 0.2f + 0.6f * h1, sy = cy + 0.2f + 0.6f * h2;
                float dx = u - sx, dy = v - sy;
                float r = sqrtf(dx * dx + dy * dy);
                if (r > 0.9f) continue;
                float ang = atan2f(dy, dx) + 0.08f * vk_sin(t * 0.003f + h1 * 9.0f) * r;
                float nn = 22.0f + floorf(h2 * 8.0f);
                float f = vk_absf(vk_fract(ang * nn / VK_TAU) - 0.5f) * (VK_TAU / nn) * r;
                float thick = 0.014f + 0.016f * (1.0f - r / 0.9f);
                float needle = vk_sstep(thick, thick * 0.3f, f) * vk_sstep(0.9f, 0.5f, r) * vk_sstep(0.0f, 0.06f, r);
                float bud = vk_sstep(0.09f, 0.03f, r);
                float val = needle > bud ? needle : bud;
                if (val > m) { m = val; ci = base + h1 * 1500.0f + r * 1800.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(u * 2.0f + v * 3.0f + t * 0.002f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
