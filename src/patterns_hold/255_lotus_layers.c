/* pattern_255 — LOTUS LAYERS (field): a lotus seen from above — rings of
 * pointed petals, each ring rotated against the last and opening slowly,
 * black gaps between the petal tips.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_255(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const int rings = 5;
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u);
            float m = 0.0f, ci = base;
            for (int k = rings - 1; k >= 0; k--) {
                int np = 8 + 4 * k;
                float open = 0.5f + 0.5f * vk_sin(t * 0.0025f + k * 0.9f);
                float r0 = 0.05f + 0.20f * k * (0.85f + 0.15f * open);
                float len = 0.30f + 0.04f * k;
                float a = ang * np / VK_TAU + (k & 1 ? 0.5f : 0.0f) + t * 0.0002f * (k & 1 ? 1 : -1);
                float fa = (vk_fract(a) - 0.5f) * (VK_TAU / np);
                float along = (r - r0) / len;                /* 0 base .. 1 tip */
                if (along < 0.0f || along > 1.0f) continue;
                float halfw = (VK_TAU / np) * 0.5f * (0.62f * vk_sin(along * 3.14159f) + 0.04f) * (0.9f + 0.1f * open);
                float across = vk_absf(fa) * (r0 + along * len) / (r0 + along * len) ;
                float pet = vk_sstep(halfw, halfw * 0.6f, vk_absf(fa));
                float vein = 0.7f + 0.3f * vk_sin(along * 12.0f + vk_absf(fa) * 30.0f);
                float grad = 0.55f + 0.45f * along;
                (void)across;
                float val = pet * vein * grad;
                if (val > m) { m = val; ci = base + k * 500.0f + along * 1400.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, 0.5f + 0.5f * vk_sin(r * 8.0f + t * 0.002f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
