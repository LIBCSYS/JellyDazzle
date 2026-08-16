/* pattern_289 — SEA ANEMONE (field): a great anemone seen from above, thick
 * tentacles radiating from the mouth and waving in the surge, tips glowing;
 * black water between the tentacles.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_289(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float nt = 26.0f + 8.0f * vk_seedf(seed, 1);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u);
            float m = 0.0f, ci = base;
            for (int L = 0; L < 2; L++) {
                float n = nt * (L ? 0.6f : 1.0f);
                float wave = 0.25f * r * vk_sin(r * 5.0f - t * 0.005f + L * 2.0f) + 0.08f * vk_sin(t * 0.003f + L);
                float a = ang * n / VK_TAU + wave + (L ? 0.5f : 0.0f);
                float f = (vk_fract(a) - 0.5f) * (VK_TAU / n);
                float across = r * vk_sin(f);
                float len = L ? 0.75f : 1.15f;
                float half = 0.055f * (1.0f - r / len * 0.5f) * (L ? 0.8f : 1.0f);
                float tent = vk_sstep(half, half * 0.4f, vk_absf(across)) * vk_sstep(len, len - 0.15f, r) * vk_sstep(0.05f, 0.18f, r);
                float shade = 0.5f + 0.5f * (1.0f - vk_absf(across) / half);
                float tip = vk_sstep(len - 0.35f, len - 0.1f, r);
                float val = tent * (0.55f + 0.45f * shade) * (L ? 0.75f : 1.0f);
                if (val > m) { m = val; ci = base + L * 900.0f + r * 1500.0f + tip * 1500.0f; }
            }
            float mouth = vk_sstep(0.18f, 0.06f, r) * 0.7f;
            if (mouth > m) { m = mouth; ci = base + 2600.0f; }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(r * 9.0f - t * 0.004f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
