/* pattern_293 — LANTERN ROWS (field): strings of round paper lanterns hung
 * in rows, ribbed and glowing from within, swinging gently out of phase;
 * night black between them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_293(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float rows = 3.5f, cols = 5.0f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * rows;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cols;
            float m = 0.0f, ci = base;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            for (int j = -1; j <= 0; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float ph = vk_h2(cx, cy, seed) * VK_TAU;
                float swing = 0.12f * vk_sin(t * 0.004f + ph);
                float sx = cx + 0.5f + ((cy & 1) ? 0.5f : 0.0f) + swing, sy = cy + 0.55f + 0.03f * vk_cos(t * 0.004f + ph);
                float dx = (u - sx) * 1.0f, dy = (v - sy) * 1.15f;
                float rad = 0.46f;
                float d = sqrtf(dx * dx + dy * dy) / rad;
                /* string above the lantern */
                float str = vk_sstep(0.03f, 0.008f, vk_absf(u - sx)) * vk_sstep(sy - rad * 0.9f, sy - rad * 0.9f - 0.02f, v) * vk_sstep(sy - 0.6f, sy - 0.5f, v) * 0.4f;
                if (str > m) { m = str; ci = base + 500.0f; }
                if (d > 1.0f) continue;
                float rib = 0.75f + 0.25f * vk_cos(dx / rad * 9.0f);
                float glow = 0.5f + 0.5f * (1.0f - d * d);
                float lit = 0.75f + 0.25f * vk_sin(t * 0.003f + ph * 3.0f);
                float val = vk_sstep(1.0f, 0.88f, d) * rib * glow * lit;
                if (val > m) { m = val; ci = base + vk_h2(cx, cy, seed ^ 3u) * 2200.0f + d * 700.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(u + v * 2.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
