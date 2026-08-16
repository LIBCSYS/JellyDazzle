/* pattern_285 — DUNE PERSPECTIVE (field): a sea of dune ridges receding to
 * a far horizon, crests lit and slip faces black, ripples on the near
 * ridges, all creeping slowly.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_285(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float horizon = 0.20f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            if (v <= horizon + 0.005f) { vk_putp(row + x * 3, 0xFF000000u); continue; }
            /* perspective ground: depth z from v */
            float z = 0.25f / (v - horizon);
            float gx = u * z;
            float wander = 0.6f * vk_noise2(gx * 0.5f, z * 0.3f + t * 0.0003f, seed) + 0.3f * vk_noise2(gx * 1.5f, z * 0.8f, seed ^ 3u);
            float p = z * 3.0f + wander * 1.5f - t * 0.002f;
            float f = vk_fract(p);
            float lit = f < 0.7f ? f / 0.7f : 1.0f - (f - 0.7f) / 0.3f;
            float m = vk_sstep(0.10f, 0.75f, lit);
            /* ripples on the near dunes */
            float rip = 0.85f + 0.15f * vk_sin(gx * 25.0f + z * 6.0f + wander * 8.0f);
            m *= rip;
            /* haze toward horizon */
            m *= vk_sstep(horizon, horizon + 0.12f, v);
            float ci = base + lit * 1400.0f + z * 300.0f + wander * 500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, rip, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
