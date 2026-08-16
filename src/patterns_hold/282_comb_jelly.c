/* pattern_282 — COMB JELLY (field): a great ctenophore filling the frame,
 * translucent body ribbed by eight comb rows down which waves of colour run;
 * the water around it black.  Mirror symmetry.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_282(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 2.0f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = vk_absf((float)x / (float)sw - 0.5f) * 2.0f;
            /* body: an egg, wider below, drifting */
            float vv = v + 0.05f * vk_sin(t * 0.0018f);
            float wdt = 0.78f * (0.75f + 0.25f * vk_sstep(-1.0f, 0.4f, vv)) * (1.0f + 0.03f * vk_sin(t * 0.005f));
            float ell = (u / wdt) * (u / wdt) + (vv / 0.98f) * (vv / 0.98f);
            float body = vk_sstep(1.0f, 0.85f, ell);
            /* comb rows: 4 per side, meridians of the egg */
            float meridian = asinf(vk_clamp01(u / (wdt + 1e-3f)));    /* 0 centre .. pi/2 edge */
            float rowp = meridian * 4.0f / 1.5708f;
            float fr = vk_absf(vk_fract(rowp) - 0.5f);
            float comb = vk_sstep(0.26f, 0.08f, fr) * vk_sstep(0.9f, 0.6f, ell);
            /* running rainbow: waves along v on each comb row */
            float wave = 0.5f + 0.5f * vk_sin(vv * 18.0f - t * 0.03f + floorf(rowp) * 1.5f);
            float glow = 0.26f + 0.2f * (1.0f - ell);
            float m = body * (glow + comb * (0.5f + 0.5f * wave));
            float ci = base + wave * 3200.0f + ell * 800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, comb, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
