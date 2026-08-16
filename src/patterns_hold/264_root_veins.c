/* pattern_264 — ROOT VEINS (field): roots reaching down from the top edge —
 * thick trunks that wander and split into finer rootlets, sap pulsing
 * faintly along them; the soil between them black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NR 9
#define NC 3
void pattern_264(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    /* per-root parameters */
    float rx0[NR], ra[NR], rb[NR], rp[NR], cw[NR][NC], cy0[NR][NC], cd[NR][NC];
    for (int i = 0; i < NR; i++) {
        rx0[i] = (i + 0.5f) / NR * 1.333f + 0.06f * (vk_seedf(seed, i) - 0.5f);
        ra[i] = 0.05f + 0.06f * vk_seedf(seed, i + 10);
        rb[i] = 2.0f + 3.0f * vk_seedf(seed, i + 20);
        rp[i] = vk_seedf(seed, i + 30) * VK_TAU;
        for (int c = 0; c < NC; c++) {
            cy0[i][c] = 0.15f + 0.6f * vk_seedf(seed, i * 7 + c + 40);
            cd[i][c] = (vk_seedf(seed, i * 7 + c + 50) < 0.5f ? -1.0f : 1.0f) * (0.15f + 0.25f * vk_seedf(seed, i * 7 + c + 60));
            cw[i][c] = 3.0f + 4.0f * vk_seedf(seed, i * 7 + c + 70);
        }
    }
    const float sway = 0.01f * vk_sin(t * 0.002f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float m = 0.0f, ci = base;
            for (int i = 0; i < NR; i++) {
                float cx = rx0[i] + ra[i] * vk_sin(v * rb[i] + rp[i] + t * 0.0008f) + sway * v * (i & 1 ? 1 : -1);
                float wdt = 0.05f * (1.0f - v * 0.6f) + 0.008f;
                float d = vk_absf(u - cx);
                float trunk = vk_sstep(wdt, wdt * 0.4f, d) * (0.6f + 0.4f * (1.0f - d / wdt));
                if (trunk > m) { m = trunk; ci = base + i * 300.0f + v * 1400.0f; }
                for (int c = 0; c < NC; c++) {
                    float dv = v - cy0[i][c];
                    if (dv < 0.0f || dv > 0.7f) continue;
                    /* child leaves the trunk with a curve, thins to nothing */
                    float ccx = cx + cd[i][c] * (1.0f - vk_cos(dv * cw[i][c])) * 0.5f;
                    float cwd = wdt * 0.7f * (1.0f - dv / 0.7f) + 0.004f;
                    float dd = vk_absf(u - ccx);
                    float child = vk_sstep(cwd, cwd * 0.4f, dd) * 0.85f;
                    if (child > m) { m = child; ci = base + i * 300.0f + 1000.0f + v * 1400.0f; }
                }
            }
            m *= vk_sstep(0.0f, 0.02f, v);
            float sap = 0.8f + 0.2f * vk_sin(v * 24.0f - t * 0.012f);
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, sap, m * sap));
        }
    }
    vk_blit(&cv, fb, w, h);
}
