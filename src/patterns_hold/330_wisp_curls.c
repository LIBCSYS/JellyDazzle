/* pattern_330 — WISP CURLS (field): a field of curling wisps — spiral
 * tendrils of smoke unwinding from scattered points, thick at the root and
 * fading to a fine tip; black between them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_330(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = 3.0f;
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
                float sx = cx + 0.25f + 0.5f * h1, sy = cy + 0.25f + 0.5f * h2;
                float dx = u - sx, dy = v - sy;
                float r = sqrtf(dx * dx + dy * dy);
                if (r > 0.9f) continue;
                float ang = atan2f(dy, dx);
                float dir = h3 < 0.5f ? 1.0f : -1.0f;
                /* spiral: r = a * theta ; distance to spiral in turns */
                float turns = (ang * dir) / VK_TAU + h1 + t * 0.0004f * dir;
                float sp = r * 3.2f - turns;             /* spiral index */
                float fs = vk_absf(vk_fract(sp) - 0.5f) / 3.2f;
                float wdt = 0.12f * (1.0f - r / 0.9f) + 0.015f;
                float wisp = vk_sstep(wdt, wdt * 0.3f, fs) * vk_sstep(0.9f, 0.5f, r) * vk_sstep(0.0f, 0.08f, r);
                float soft = 0.7f + 0.3f * vk_noise2(sp * 3.0f, cx * 7 + cy, seed);
                float val = wisp * soft;
                if (val > m) { m = val; ci = base + h1 * 1500.0f + r * 1500.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, 0.5f + 0.5f * vk_sin(u * 2.0f + v * 3.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
