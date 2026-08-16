/* pattern_257 — LICHEN PATCHES (field): crustose lichen on dark rock —
 * roundish patches with lobed rims growing outward, overlapping, each patch
 * its own colour, breathing very slowly.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_257(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = 4.5f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float m = 0.0f, ci = base;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float h1 = vk_h2(cx, cy, seed), h2 = vk_h2(cx, cy, seed ^ 3u), h3 = vk_h2(cx, cy, seed ^ 5u);
                float sx = cx + 0.2f + 0.6f * h1, sy = cy + 0.2f + 0.6f * h2;
                float dx = u - sx, dy = v - sy;
                float d = sqrtf(dx * dx + dy * dy);
                float a = atan2f(dy, dx);
                float grow = 0.62f + 0.18f * vk_sin(t * 0.0015f + h3 * VK_TAU);
                float rr = grow * (0.85f + 0.15f * vk_sin(a * (5.0f + floorf(h3 * 4.0f)) + h1 * 9.0f) + 0.05f * vk_sin(a * 17.0f));
                if (h3 < 0.12f) continue;                       /* bare rock cell */
                float patch = vk_sstep(rr, rr - 0.08f, d);
                float rim = vk_sstep(0.10f, 0.02f, vk_absf(d - rr + 0.06f));
                float tex = 0.75f + 0.25f * vk_noise2(u * 12.0f, v * 12.0f, seed + cx * 7 + cy);
                float val = patch * tex * (0.7f + 0.3f * rim);
                if (val > m) { m = val; ci = base + h1 * 2600.0f + d * 600.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.4f, ci + 1300.0f, 0.5f + 0.5f * vk_sin(u * 3.0f + v * 2.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
