/* pattern_250 — CHAIN MAIL (field): rings linked four-in-one, each ring a
 * shaded torus catching a wandering light, the mesh flexing; the holes in
 * the mail are black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_250(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 5.0f, 8.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float a = (u + 0.05f * vk_sin(v * 3.0f + t * 0.002f)) * n;
            float b = (v + 0.05f * vk_sin(u * 2.0f - t * 0.0015f)) * n * 1.15f;
            float m = 0.0f, ci = base;
            /* rings on a staggered grid, radius ~0.62 cell so neighbours overlap */
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                float ry = floorf(b) + j;
                float rx = floorf(a) + i + (((int)ry & 1) ? 0.5f : 0.0f);
                float dx = a - rx, dy = (b - ry) * 1.15f;
                float d = sqrtf(dx * dx + dy * dy);
                float ring = vk_sstep(0.16f, 0.06f, vk_absf(d - 0.62f));
                float tor = 0.6f + 0.4f * vk_cos((d - 0.62f) * 18.0f);
                float lit = 0.6f + 0.4f * vk_sin(atan2f(dy, dx) + t * 0.003f);
                float val = ring * tor * lit;
                if (val > m) { m = val; ci = base + vk_h2((int)rx * 2, (int)ry, seed) * 700.0f + lit * 1500.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(u * 2.0f + v * 3.0f + t * 0.002f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
