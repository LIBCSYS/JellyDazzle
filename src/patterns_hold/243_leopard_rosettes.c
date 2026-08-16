/* pattern_243 — LEOPARD ROSETTES (field): a pelt of rosettes — broken rings
 * of colour with a warm centre, scattered on black, the pelt rippling as the
 * animal breathes.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_243(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = vk_seedr(seed, 1, 6.0f, 9.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells + 0.15f * vk_sin(v * 1.5f + t * 0.002f);
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float m = 0.0f, ci = base;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float h1 = vk_h2(cx, cy, seed), h2 = vk_h2(cx, cy, seed ^ 3u);
                float sx = cx + 0.25f + 0.5f * h1, sy = cy + 0.25f + 0.5f * h2;
                float dx = u - sx, dy = (v - sy) * (0.8f + 0.4f * h1);
                float d = sqrtf(dx * dx + dy * dy);
                float rr = 0.40f + 0.08f * vk_sin(t * 0.003f + h1 * 20.0f);
                float ring = vk_sstep(0.14f, 0.05f, vk_absf(d - rr));
                /* break the ring into 2-3 lobes */
                float a = atan2f(dy, dx);
                float lobe = vk_sstep(0.0f, 0.25f, 0.5f + 0.5f * vk_sin(a * (2.0f + floorf(h2 * 2.0f)) + h1 * 9.0f));
                float core = vk_sstep(rr * 0.65f, rr * 0.25f, d) * 0.8f;
                float val = ring * lobe > core ? ring * lobe : core;
                if (val > m) { m = val; ci = base + h1 * 900.0f + (core > ring * lobe ? 2400.0f : 0.0f) + d * 800.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, 0.5f + 0.5f * vk_sin(u * 2.0f + v * 3.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
