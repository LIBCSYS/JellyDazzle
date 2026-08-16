/* pattern_238 — VORTEX SHEET (field): a sheet of fluid wound into a slow
 * vortex, its layers stacked as bright spiral bands with black water between
 * them; the sheet drifts and the vortex breathes.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_238(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float arms = 3.0f + (float)(seed % 3u);
    const float cx = 0.15f * vk_sin(t * 0.0008f), cy = 0.12f * vk_cos(t * 0.0006f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f - cy;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f - cx;
            float r = sqrtf(u * u + v * v) + 1e-3f;
            float ang = atan2f(v, u);
            /* wind-up: angle advances faster near the centre */
            float ph = ang * arms + 6.0f / (r + 0.25f) + t * 0.004f - r * 4.0f;
            float band = 0.5f + 0.5f * vk_sin(ph);
            float wob = vk_noise2(u * 3.0f + t * 0.001f, v * 3.0f, seed);
            float m = vk_sstep(0.35f, 0.75f, band + 0.2f * (wob - 0.5f));
            m *= vk_sstep(1.35f, 0.9f, r) * (0.7f + 0.3f * vk_sstep(0.0f, 0.5f, r));
            float ci = base + r * 2400.0f + band * 700.0f + wob * 600.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1700.0f, wob, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
