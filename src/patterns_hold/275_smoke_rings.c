/* pattern_275 — SMOKE RINGS (field): fat smoke rings drift up and expand,
 * each a soft torus with a dark hole, thinning as it grows; black air
 * between them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NSR 9
void pattern_275(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float cx[NSR], cy[NSR], rr[NSR], th[NSR], ph[NSR];
    for (int i = 0; i < NSR; i++) {
        float life = vk_fract(vk_seedf(seed, i) + t * 0.00025f * (0.7f + 0.6f * vk_seedf(seed, i + 9)));
        cx[i] = 0.15f + 1.05f * vk_seedf(seed, i + 20) + 0.05f * vk_sin(t * 0.001f + i);
        cy[i] = 1.1f - life * 1.3f;
        rr[i] = 0.10f + 0.34f * life;
        th[i] = 0.14f * (1.0f - life * 0.4f);
        ph[i] = vk_seedf(seed, i + 30) * VK_TAU;
    }
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            uint32_t col = 0xFF000000u;
            for (int i = 0; i < NSR; i++) {
                float dx = u - cx[i], dy = (v - cy[i]) * 1.6f;     /* rings seen at an angle */
                float d = sqrtf(dx * dx + dy * dy);
                float ang = atan2f(dy, dx);
                float wob = 1.0f + 0.06f * vk_sin(ang * 3.0f + t * 0.004f + ph[i]) + 0.03f * vk_sin(ang * 7.0f - t * 0.003f);
                float e = vk_absf(d - rr[i] * wob);
                if (e > th[i]) continue;
                float body = vk_sstep(th[i], th[i] * 0.3f, e);
                float tex = 0.7f + 0.3f * vk_noise2(ang * 4.0f + i * 5.0f, d * 8.0f + t * 0.003f, seed);
                float m = body * tex * (0.55f + 0.45f * (1.0f - rr[i] / 0.4f));
                float ci = base + i * 500.0f + e / th[i] * 1200.0f + t * 0.5f;
                col = vk_max(col, vk_pc2(pal, ci, ci + 1600.0f, tex, m));
            }
            vk_putp(row + x * 3, col);
        }
    }
    vk_blit(&cv, fb, w, h);
}
