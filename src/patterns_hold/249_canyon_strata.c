/* pattern_249 — CANYON STRATA (field): layered rock — undulating horizontal
 * strata in bands of colour, dark seams between the layers and black
 * fissures cutting down through them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_249(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float nl = vk_seedr(seed, 1, 9.0f, 14.0f);
    const float tz = t * 0.001f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float fold = 0.08f * vk_sin(u * 3.0f + tz * 2.0f) + 0.04f * vk_noise2(u * 4.0f, v * 2.0f + tz, seed);
            float lv = (v + fold) * nl;
            int il = (int)floorf(lv);
            float f = vk_fract(lv);
            float thick = 0.6f + 0.35f * vk_h2(il, 0, seed);
            float layer = vk_sstep(0.0f, 0.10f, f) * vk_sstep(thick, thick - 0.12f, f);
            /* fissures: vertical-ish black cracks wandering */
            float crack = vk_noise2(u * 6.0f + v * 0.5f, tz * 0.5f + il * 0.1f, seed ^ 7u);
            float fis = vk_sstep(0.03f, 0.08f, vk_absf(crack - 0.5f));
            float grain = 0.8f + 0.2f * vk_sin(u * 60.0f + f * 20.0f);
            float m = layer * fis * grain * (0.6f + 0.4f * vk_h2(il, 1, seed));
            float ci = base + vk_h2(il, 2, seed) * 2200.0f + f * 500.0f + t * 0.4f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
