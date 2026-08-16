/* pattern_213 — PLUME FEATHERS (field): a spray of soft ostrich plumes
 * fanning up from below, barbs waving; black between the plumes.  Mirror
 * symmetry.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_213(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const int np = 4 + (int)(vk_seedf(seed, 1) * 2.0f);
    for (int y = 0; y < sh; y++) {
        float v = 1.2f - (float)y / (float)sh;              /* up from bottom */
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = vk_absf((float)x / (float)sw - 0.5f) * 2.6f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(u, v);
            float sway = 0.07f * vk_sin(r * 2.5f + t * 0.003f);
            float pa = (ang + sway) * (float)np / (VK_TAU * 0.25f) + 0.5f;
            float f = vk_fract(pa) - 0.5f;
            /* plume: soft body, feathery edge made by low-freq noise on the outline */
            float edge = 0.10f * vk_noise2(r * 6.0f, floorf(pa) * 3.0f + t * 0.002f, seed);
            float width = (0.40f + edge) * vk_sstep(0.05f, 0.40f, r) * vk_sstep(1.7f, 0.8f, r);
            float body = vk_sstep(width, width * 0.15f, vk_absf(f));
            float barb = 0.72f + 0.28f * vk_sin(f * 26.0f + r * 22.0f - t * 0.008f);
            float down = 0.85f + 0.15f * vk_sin(r * 60.0f + f * 10.0f);
            float m = body * barb * down * vk_sstep(0.05f, 0.25f, r) * vk_sstep(1.75f, 1.45f, vk_absf(ang));
            float ci = base + r * 2000.0f + vk_absf(f) * 2400.0f + t * 0.7f;
            float cj = ci + 1500.0f + floorf(pa) * 500.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, barb, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
