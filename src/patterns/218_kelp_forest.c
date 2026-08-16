/* pattern_218 — KELP FOREST (field): tall kelp fronds sway from the sea
 * floor, blades overlapping, dark water between; light filters from above.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NK 9
void pattern_218(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float kx[NK], kw[NK], kp[NK], ks[NK];
    for (int i = 0; i < NK; i++) {
        kx[i] = (i + 0.5f + 0.6f * (vk_seedf(seed, i * 4 + 1) - 0.5f)) / NK * 1.333f;
        kw[i] = 0.05f + 0.045f * vk_seedf(seed, i * 4 + 2);
        kp[i] = vk_seedf(seed, i * 4 + 3) * VK_TAU;
        ks[i] = 0.7f + 0.6f * vk_seedf(seed, i * 4 + 4);
    }
    for (int y = 0; y < sh; y++) {
        float v = 1.0f - (float)y / (float)sh;      /* 0 = floor */
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            uint32_t col = 0xFF000000u;
            for (int i = 0; i < NK; i++) {
                /* stipe sways: lateral offset grows with height */
                float off = 0.10f * v * v * vk_sin(t * 0.0025f * ks[i] + kp[i]) + 0.03f * v * vk_sin(v * 6.0f - t * 0.004f + kp[i]);
                float dx = u - kx[i] - off;
                /* blades: width oscillates along the stipe */
                float wdt = kw[i] * (0.6f + 0.6f * vk_absf(vk_sin(v * 9.0f + kp[i] * 3.0f)));
                float ad = vk_absf(dx);
                if (ad > wdt) continue;
                float m = vk_sstep(wdt, wdt * 0.5f, ad) * vk_sstep(1.15f, 0.9f, v) * vk_sstep(-0.05f, 0.05f, v);
                float vein = 0.75f + 0.25f * vk_sin(ad / wdt * 12.0f + v * 20.0f);
                float ci = base + i * 350.0f + v * 2000.0f + ad / wdt * 900.0f + t * 0.5f;
                col = vk_max(col, vk_pc2(pal, ci, ci + 1600.0f, vein, m * vein));
            }
            vk_putp(row + x * 3, col);
        }
    }
    vk_blit(&cv, fb, w, h);
}
