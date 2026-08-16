/* pattern_283 — CORAL POLYPS (field): a reef surface crowded with polyps,
 * each a soft disc of tentacles opening and closing on its own clock; the
 * rock between them black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_283(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = 7.0f;
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
                float sx = cx + 0.25f + 0.5f * h1, sy = cy + 0.25f + 0.5f * h2;
                float dx = u - sx, dy = v - sy;
                float r = sqrtf(dx * dx + dy * dy);
                float open = 0.5f + 0.5f * vk_sin(t * 0.003f + h1 * VK_TAU);
                float rad = 0.35f + 0.25f * open;
                if (r > rad) continue;
                float ang = atan2f(dy, dx);
                float nt = 10.0f + floorf(h2 * 6.0f);
                float tent = 0.5f + 0.5f * vk_sin(ang * nt + r * 8.0f * open + t * 0.004f);
                float disc = vk_sstep(rad, rad * 0.5f, r);
                float mouth = vk_sstep(0.12f, 0.04f, r) * 0.8f;
                float val = disc * (0.35f + 0.65f * vk_sstep(0.3f, 0.8f, tent)) * (0.5f + 0.5f * open);
                val = val > mouth ? val : mouth;
                if (val > m) { m = val; ci = base + h1 * 1800.0f + r * 1500.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(u + v * 2.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
