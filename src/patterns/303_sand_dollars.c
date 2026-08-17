/* pattern_303 — SAND DOLLARS (field): sand dollars scattered on dark sand,
 * each a soft disc with the five-petal rosette pressed into it and a fringe
 * of fine ridges, drifting slowly.  5-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_303(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = 2.8f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells + t * 0.0002f;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float m = 0.0f, ci = base;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float h1 = vk_h2(cx, cy, seed), h2 = vk_h2(cx, cy, seed ^ 3u);
                float sx = cx + 0.25f + 0.5f * h1, sy = cy + 0.25f + 0.5f * h2;
                float dx = u - sx, dy = v - sy;
                float rad = 0.44f + 0.1f * vk_h2(cx, cy, seed ^ 5u);
                float d = sqrtf(dx * dx + dy * dy) / rad;
                if (d > 1.0f) continue;
                float ang = atan2f(dy, dx) + h1 * VK_TAU + 0.02f * vk_sin(t * 0.002f + h2 * 9.0f);
                float disc = vk_sstep(1.0f, 0.9f, d) * (0.5f + 0.2f * (1.0f - d));
                /* five petals: elongated lobes */
                float a5 = vk_absf(vk_fract(ang * 5.0f / VK_TAU + 0.5f) - 0.5f) * (VK_TAU / 5.0f);
                float px = d * vk_cos(a5), py = d * vk_sin(a5);
                float pet = vk_sstep(1.0f, 0.7f, ((px - 0.35f) / 0.32f) * ((px - 0.35f) / 0.32f) + (py / 0.11f) * (py / 0.11f)) * 0.5f;
                float ridge = 0.85f + 0.15f * vk_sin(ang * 60.0f) * vk_sstep(0.6f, 0.9f, d);
                float val = (disc + pet) * ridge;
                if (val > m) { m = val > 1.0f ? 1.0f : val; ci = base + h1 * 900.0f + pet * 2000.0f + d * 500.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(u * 2.0f + v * 3.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
