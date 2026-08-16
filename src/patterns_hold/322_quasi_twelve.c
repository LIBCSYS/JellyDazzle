/* pattern_322 — QUASI TWELVE (field): twelve plane waves summed into a
 * dodecagonal quasicrystal — rosettes within rosettes, bright where the
 * waves agree and black where they cancel, drifting slowly.  12-fold.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_322(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float k = vk_seedr(seed, 1, 10.0f, 15.0f);
    float kx[6], ky[6], ph[6];
    for (int i = 0; i < 6; i++) {
        float a = i * VK_TAU / 12.0f + t * 0.00015f;
        kx[i] = vk_cos(a) * k; ky[i] = vk_sin(a) * k;
        ph[i] = t * 0.003f * (1.0f + 0.15f * i) + vk_seedf(seed, i) * VK_TAU;
    }
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float s = 0.0f;
            for (int i = 0; i < 6; i++) s += vk_cos(u * kx[i] + v * ky[i] + ph[i]);
            float f = s / 12.0f + 0.5f;
            float m = vk_sstep(0.44f, 0.72f, f) ;
            float lace = vk_sstep(0.05f, 0.0f, vk_absf(f - 0.30f)) * 0.6f;
            m = m > lace ? m : lace;
            float ci = base + f * 3200.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
