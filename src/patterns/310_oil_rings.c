/* pattern_310 — OIL RINGS (field): drops of oil on dark water, each spreading
 * rings of interference colour that overlap and beat, the water between the
 * films black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define ND 5
void pattern_310(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float cx[ND], cy[ND], rr[ND];
    for (int i = 0; i < ND; i++) {
        cx[i] = 0.15f + 1.05f * vk_seedf(seed, i) + 0.04f * vk_sin(t * 0.0008f + i);
        cy[i] = 0.15f + 0.7f * vk_seedf(seed, i + 8) + 0.03f * vk_cos(t * 0.0006f + i * 2.0f);
        rr[i] = 0.26f + 0.12f * vk_seedf(seed, i + 16) + 0.04f * vk_sin(t * 0.0015f + i);
    }
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float thick = 0.0f, cover = 0.0f;
            for (int i = 0; i < ND; i++) {
                float dx = u - cx[i], dy = v - cy[i];
                float d = sqrtf(dx * dx + dy * dy) / rr[i];
                if (d > 1.0f) continue;
                float film = vk_sstep(1.0f, 0.85f, d);
                thick += film * (1.0f - d * d) * (0.8f + 0.2f * vk_sin(t * 0.003f + i));
                cover = cover > film ? cover : film;
            }
            float band = thick * 6.0f + t * 0.002f;
            float fr = vk_fract(band);
            float m = cover * (0.15f + 0.85f * vk_sstep(0.2f, 0.75f, 0.5f + 0.5f * vk_sin(band * VK_TAU)));
            float ci = base + fr * 3000.0f + thick * 500.0f + t * 0.4f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, fr, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
