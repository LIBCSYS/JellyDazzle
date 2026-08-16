/* pattern_247 — PEARL CURTAIN (field): strings of beads hanging in a
 * doorway, swaying out of phase, each bead a small lit sphere; the doorway
 * behind is black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_247(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float ns = vk_seedr(seed, 1, 10.0f, 14.0f);
    const float nbead = 10.0f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            uint32_t col = 0xFF000000u;
            /* two nearest strings */
            float su = u * ns;
            for (int k = -1; k <= 1; k++) {
                int is = (int)floorf(su) + k;
                float ph = vk_h2(is, 0, seed) * VK_TAU;
                float sway = 0.35f * v * vk_sin(t * 0.004f + ph) + 0.15f * v * v * vk_sin(t * 0.0027f + ph * 2.0f);
                float dx = (su - (is + 0.5f)) - sway;
                if (vk_absf(dx) > 0.7f) continue;
                float bv = v * nbead + vk_h2(is, 1, seed);
                float fy = (vk_fract(bv) - 0.5f) / nbead * ns;   /* bead-space y */
                float d = sqrtf(dx * dx + fy * fy * 1.1f);
                float rad = 0.46f;
                float bead = vk_sstep(rad, rad * 0.75f, d);
                float hi = vk_sstep(rad, 0.0f, sqrtf((dx + 0.1f) * (dx + 0.1f) + (fy + 0.1f) * (fy + 0.1f)));
                float thread = vk_sstep(0.05f, 0.015f, vk_absf(dx)) * 0.35f;
                float m = bead * (0.55f + 0.45f * hi);
                m = m > thread ? m : thread;
                m *= vk_sstep(0.0f, 0.04f, v) * vk_sstep(1.02f, 0.85f + 0.1f * vk_h2(is, 2, seed), v);
                float ci = base + floorf(bv) * 260.0f + vk_h2(is, 3, seed) * 700.0f + t * 0.5f;
                col = vk_max(col, vk_pc2(pal, ci, ci + 1500.0f, hi, m));
            }
            vk_putp(row + x * 3, col);
        }
    }
    vk_blit(&cv, fb, w, h);
}
