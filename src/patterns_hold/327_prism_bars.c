/* pattern_327 — PRISM BARS (field): a row of tall glass prisms standing
 * side by side, each splitting the light into bands of colour that slide
 * up and down its face, dark gaps between the bars.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_327(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float nb = vk_seedr(seed, 1, 7.0f, 11.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * nb + 0.02f * vk_sin(v * 5.0f + t * 0.002f);
            int ib = (int)floorf(u);
            float f = vk_fract(u) - 0.5f;
            float half = 0.36f + 0.05f * vk_sin(t * 0.0015f + ib);
            float d = vk_absf(f) / half;
            if (d > 1.0f) { vk_putp(row + x * 3, 0xFF000000u); continue; }
            /* three faces of the prism: left, front, right */
            float face = f < -half * 0.33f ? 0.6f : f > half * 0.33f ? 0.75f : 1.0f;
            float bandv = v * 3.0f + f * 1.5f + t * 0.002f * (ib & 1 ? 1 : -1) + vk_h2(ib, 0, seed) * 4.0f;
            float spectrum = vk_fract(bandv);
            float bright = 0.5f + 0.5f * vk_sin(bandv * VK_TAU);
            float m = vk_sstep(1.0f, 0.9f, d) * face * (0.4f + 0.6f * vk_sstep(0.2f, 0.7f, bright)) * vk_sstep(0.0f, 0.05f, v) * vk_sstep(1.0f, 0.95f, v);
            /* facet edge glints */
            float edge = vk_sstep(0.04f, 0.0f, vk_absf(vk_absf(f) - half * 0.33f)) * 0.4f;
            m = m > edge ? m : edge;
            float ci = base + spectrum * 3200.0f + t * 0.4f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 800.0f, face, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
