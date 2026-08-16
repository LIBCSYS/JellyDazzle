/* pattern_309 — SMOKE COLUMNS (field): columns of smoke rising from several
 * sources along the bottom edge, widening and curling as they climb, black
 * air between them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NSC 5
void pattern_309(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.002f;
    for (int y = 0; y < sh; y++) {
        float v = 1.0f - (float)y / (float)sh;      /* 0 = bottom */
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            uint32_t col = 0xFF000000u;
            for (int i = 0; i < NSC; i++) {
                float sx = (i + 0.5f) / NSC * 1.333f + 0.08f * (vk_seedf(seed, i) - 0.5f);
                float drift = 0.15f * v * v * vk_sin(t * 0.0015f + i * 1.7f) + 0.06f * v * vk_sin(v * 5.0f - t * 0.004f + i);
                float dx = u - sx - drift;
                float wdt = 0.05f + 0.22f * v;
                if (vk_absf(dx) > wdt) continue;
                float prof = vk_sstep(wdt, wdt * 0.3f, vk_absf(dx));
                float dens = vk_fbm3(dx * 8.0f + i * 5.0f, v * 4.0f - tz * 1.5f, tz, 3, seed + i);
                float m = prof * vk_sstep(0.35f, 0.7f, dens) * (0.5f + 0.5f * dens) * vk_sstep(1.15f, 0.7f, v) * vk_sstep(0.0f, 0.05f, v);
                float ci = base + i * 500.0f + v * 1500.0f + dens * 800.0f + t * 0.5f;
                col = vk_max(col, vk_pc2(pal, ci, ci + 1600.0f, dens, m));
            }
            vk_putp(row + x * 3, col);
        }
    }
    vk_blit(&cv, fb, w, h);
}
