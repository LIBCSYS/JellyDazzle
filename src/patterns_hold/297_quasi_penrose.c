/* pattern_297 — QUASI PENROSE (field): five plane waves summed into a
 * quasicrystal — pentagonal stars and rosettes that never quite repeat,
 * bright where the waves agree, black where they cancel, the whole lattice
 * drifting.  5-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_297(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float k = vk_seedr(seed, 1, 9.0f, 14.0f);
    float kx[5], ky[5], ph[5];
    for (int i = 0; i < 5; i++) {
        float a = i * VK_TAU / 5.0f + t * 0.0002f;
        kx[i] = vk_cos(a) * k; ky[i] = vk_sin(a) * k;
        ph[i] = t * 0.004f * (1.0f + 0.2f * i) + vk_seedf(seed, i) * VK_TAU;
    }
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float s = 0.0f;
            for (int i = 0; i < 5; i++) s += vk_cos(u * kx[i] + v * ky[i] + ph[i]);
            /* s in -5..5 */
            float f = s * 0.1f + 0.5f;
            float m = vk_sstep(0.5f, 0.8f, f);
            float ring = vk_sstep(0.06f, 0.0f, vk_absf(f - 0.32f)) * 0.6f;
            m = m > ring ? m : ring;
            float ci = base + f * 3000.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
