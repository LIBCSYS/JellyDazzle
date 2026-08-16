/* pattern_239 — FUNGAL GILLS (field): looking up under a mushroom cap —
 * radial lamellae fanning from the stem, splitting as they reach the rim,
 * black shadow between the gills; the cap breathes.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_239(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n0 = 20.0f + 8.0f * vk_seedf(seed, 1);
    const float rot = t * 0.0004f;
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u) + rot + 0.05f * vk_sin(r * 6.0f + t * 0.003f);
            /* gill count doubles at r=0.45 and again at 0.85 */
            float m = 0.0f;
            float lvl = 1.0f;
            for (int o = 0; o < 3; o++) {
                float n = n0 * lvl;
                float f = vk_absf(vk_fract(ang * n / VK_TAU + (o & 1 ? 0.5f : 0.0f)) - 0.5f) * (VK_TAU / n) * r;
                float thick = 0.016f + 0.014f * vk_sstep(0.0f, 1.2f, r);
                float g = vk_sstep(thick, thick * 0.35f, f);
                float r0 = o == 0 ? 0.08f : o == 1 ? 0.45f : 0.85f;
                float band = vk_sstep(r0 - 0.03f, r0 + 0.05f, r);
                float val = g * band;
                if (val > m) m = val;
                lvl *= 2.0f;
            }
            /* stem disc and rim fade */
            float stem = vk_sstep(0.16f, 0.06f, r);
            m = m > stem ? m : stem;
            m *= vk_sstep(1.25f, 0.95f, r);
            float shade = 0.65f + 0.35f * vk_sin(r * 14.0f - t * 0.004f);
            float ci = base + r * 3000.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, shade, m * shade));
        }
    }
    vk_blit(&cv, fb, w, h);
}
