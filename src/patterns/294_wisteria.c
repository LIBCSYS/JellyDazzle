/* pattern_294 — WISTERIA (field): hanging racemes of blossom cascading from
 * the top edge, each cluster tapering to a tip, florets ruffled, swaying;
 * black between the clusters.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_294(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float nc = vk_seedr(seed, 1, 7.0f, 10.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float m = 0.0f, ci = base;
            for (int L = 0; L < 2; L++) {
                float uu = (u + (L ? 0.5f / nc : 0.0f)) * nc;
                int ic = (int)floorf(uu);
                float f = vk_fract(uu) - 0.5f;
                float h1 = vk_h2(ic, L, seed), h2 = vk_h2(ic, L + 5, seed);
                float len = 0.45f + 0.5f * h1;
                float top = L ? 0.1f + 0.15f * h2 : -0.05f;
                float sway = 0.15f * (v - top) * vk_sin(t * 0.0025f + h1 * VK_TAU + L);
                float dx = (f - sway) ;
                float along = (v - top) / len;
                if (along < 0.0f || along > 1.0f) continue;
                float halfw = 0.34f * (0.5f + 0.5f * sqrtf(vk_sin(along * 3.14159f))) * (1.0f - along * 0.35f) + 0.05f * vk_sin(v * 45.0f + h2 * 9.0f) + 0.03f * vk_sin(v * 90.0f);
                halfw *= vk_sstep(1.0f, 0.8f, along) * vk_sstep(0.0f, 0.1f, along);
                float body = vk_sstep(halfw, halfw * 0.5f, vk_absf(dx));
                /* florets: ruffle texture */
                float ruf = 0.55f + 0.45f * vk_noise2(dx * 12.0f + ic * 3.0f, v * 30.0f + L * 7.0f, seed);
                float val = body * (0.5f + 0.5f * ruf) * (L ? 0.8f : 1.0f);
                if (val > m) { m = val; ci = base + h1 * 800.0f + along * 1800.0f + L * 600.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, 0.5f + 0.5f * vk_sin(u * 5.0f + v * 3.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
