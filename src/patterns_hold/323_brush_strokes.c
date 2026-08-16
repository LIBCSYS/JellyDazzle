/* pattern_323 — BRUSH STROKES (field): broad diagonal strokes of a loaded
 * brush, bristle streaks running along each stroke, ragged ends, black
 * canvas showing between and through them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NST 9
void pattern_323(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float ang = vk_seedr(seed, 1, 0.4f, 1.1f);
    const float ca = vk_cos(ang), sa = vk_sin(ang);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh - 0.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f - 0.667f;
            float a = u * ca + v * sa, b = -u * sa + v * ca;    /* along, across */
            float m = 0.0f, ci = base;
            for (int i = 0; i < NST; i++) {
                float h1 = vk_seedf(seed, i + 10), h2 = vk_seedf(seed, i + 30), h3 = vk_seedf(seed, i + 50);
                float cb = (i - NST * 0.5f + 0.5f) * 0.19f + 0.03f * vk_sin(t * 0.0012f + h1 * 9.0f);
                float half = 0.06f + 0.03f * h2;
                float wobble = 0.015f * vk_sin(a * 12.0f + h3 * 20.0f + t * 0.002f);
                float d = vk_absf(b - cb - wobble);
                if (d > half) continue;
                float a0 = -0.9f + 0.6f * h1, a1 = a0 + 0.9f + 0.7f * h3;
                a0 += 0.05f * vk_sin(t * 0.0018f + i);
                float ends = vk_sstep(a0 - 0.05f, a0 + 0.1f, a) * vk_sstep(a1 + 0.05f, a1 - 0.15f, a);
                float bristle = 0.55f + 0.45f * vk_noise2(d / half * 12.0f + i * 7.0f, a * 3.0f, seed);
                float dry = vk_sstep(0.2f, 0.5f, bristle + 0.3f * (1.0f - (a - a0) / (a1 - a0)));
                float val = vk_sstep(half, half * 0.7f, d) * ends * dry * (0.6f + 0.4f * bristle);
                if (val > m) { m = val; ci = base + i * 450.0f + bristle * 800.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1600.0f, 0.5f + 0.5f * vk_sin(a * 4.0f + t * 0.002f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
