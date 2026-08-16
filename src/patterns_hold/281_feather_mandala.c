/* pattern_281 — FEATHER MANDALA (field): twelve soft feathers radiating
 * from the centre, barbs shimmering, tips curling with a slow breath; black
 * between the feathers.  12-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_281(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const int nf = 12;
    const float rot = t * 0.0004f;
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u) + rot + 0.15f * r * vk_sin(t * 0.0015f);   /* curl */
            float m = 0.0f, ci = base;
            for (int L = 0; L < 2; L++) {                     /* two rings of feathers */
                float a = ang * nf / VK_TAU + (L ? 0.5f : 0.0f);
                float f = (vk_fract(a) - 0.5f) * (VK_TAU / nf);
                float across = r * vk_sin(f), along = r * vk_cos(f);
                float r0 = L ? 0.35f : 0.05f, r1 = L ? 1.25f : 0.75f;
                float pos = (along - r0) / (r1 - r0);
                if (pos < 0.0f || pos > 1.0f) continue;
                float halfw = 0.10f * vk_sin(pos * 3.14159f) + 0.02f;
                float vane = vk_sstep(halfw, halfw * 0.5f, vk_absf(across));
                float barb = 0.6f + 0.4f * vk_sin(across * 60.0f + along * 25.0f - t * 0.006f);
                float shaft = vk_sstep(0.012f, 0.0f, vk_absf(across));
                float val = vane * (0.6f + 0.4f * barb) * (0.75f + 0.25f * shaft) * (L ? 0.85f : 1.0f);
                if (val > m) { m = val; ci = base + L * 1200.0f + pos * 1600.0f + vk_absf(across) * 4000.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, 0.5f + 0.5f * vk_sin(r * 6.0f - t * 0.003f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
