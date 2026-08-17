/* pattern_314 — TENTACLE CURTAIN (field): a curtain of long tentacles
 * hanging from the top edge, each tapering and waving with its own rhythm,
 * translucent and overlapping; black water between.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NTC 18
void pattern_314(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float x0[NTC], amp[NTC], frq[NTC], ph[NTC], wd[NTC], ln[NTC];
    for (int i = 0; i < NTC; i++) {
        x0[i] = (i + 0.5f) / NTC * 1.333f + 0.03f * (vk_seedf(seed, i) - 0.5f);
        amp[i] = 0.06f + 0.10f * vk_seedf(seed, i + 20);
        frq[i] = 3.0f + 4.0f * vk_seedf(seed, i + 40);
        ph[i] = vk_seedf(seed, i + 60) * VK_TAU;
        wd[i] = 0.035f + 0.03f * vk_seedf(seed, i + 80);
        ln[i] = 0.7f + 0.4f * vk_seedf(seed, i + 100);
    }
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            uint32_t col = 0xFF000000u;
            for (int i = 0; i < NTC; i++) {
                float cx = x0[i] + amp[i] * v * vk_sin(v * frq[i] - t * 0.004f + ph[i]) + 0.04f * v * v * vk_sin(t * 0.0015f + ph[i] * 2.0f);
                float half = wd[i] * (1.0f - v / ln[i]);
                if (half <= 0.0f) continue;
                float d = vk_absf(u - cx);
                if (d > half) continue;
                float body = vk_sstep(half, half * 0.5f, d);
                float shade = 0.55f + 0.45f * sqrtf(1.0f - (d / half) * (d / half));
                float sucker = 0.85f + 0.15f * vk_sin(v * 60.0f + i);
                float m = body * shade * sucker * vk_sstep(0.0f, 0.03f, v);
                float ci = base + i * 250.0f + v * 1500.0f + t * 0.5f;
                col = vk_max(col, vk_pc2(pal, ci, ci + 1500.0f, d / half, m));
            }
            vk_putp(row + x * 3, col);
        }
    }
    vk_blit(&cv, fb, w, h);
}
