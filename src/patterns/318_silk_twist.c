/* pattern_318 — SILK TWIST (field): tall columns of twisted silk, each a
 * helix of bright and shadowed bands turning slowly, columns swaying, black
 * between them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_318(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const int nc = 5 + (int)(vk_seedf(seed, 1) * 3.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw;
            float m = 0.0f, ci = base;
            for (int i = 0; i < nc; i++) {
                float cx = (i + 0.5f) / nc + 0.03f * vk_sin(v * 4.0f + t * 0.002f + i * 1.3f);
                float half = 0.075f + 0.02f * vk_seedf(seed, i + 5);
                float d = (u - cx) / half;
                if (vk_absf(d) > 1.0f) continue;
                /* helix: bands travelling up, phase depends on d (twist) */
                float ph = v * 14.0f + d * 2.2f + t * 0.006f * (i & 1 ? 1 : -1) + i;
                float band = 0.5f + 0.5f * vk_sin(ph);
                float body = vk_sstep(1.0f, 0.8f, vk_absf(d)) * sqrtf(1.0f - d * d * 0.8f);
                float val = body * vk_sstep(0.25f, 0.6f, band) * (0.6f + 0.4f * band);
                if (val > m) { m = val; ci = base + i * 500.0f + band * 1500.0f + v * 500.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, 0.5f + 0.5f * vk_sin(v * 6.0f - t * 0.003f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
