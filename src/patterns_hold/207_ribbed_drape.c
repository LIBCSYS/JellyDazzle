/* pattern_207 — RIBBED DRAPE (field): a heavy ribbed cloth gathered at the
 * top edge and falling in a soft catenary; ribs follow the fall, black
 * shadow between ribs, and open air below the waving hem.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_207(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float ribs = vk_seedr(seed, 5, 10.0f, 16.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw;
            /* gathers: ribs pinch together at the top and spread downward */
            float spread = 0.35f + 0.65f * v;
            float px = (u - 0.5f) / spread;
            float p = px * ribs + 0.6f * vk_sin(v * 5.0f + t * 0.0033f + u * 2.0f)
                    + 0.3f * vk_sin(v * 12.0f - t * 0.0021f) + t * 0.0018f;
            float f = vk_fract(p);
            /* rib: rounded profile with a bright crest and black valley */
            float rib = vk_sstep(0.0f, 0.32f, f) * vk_sstep(0.78f, 0.50f, f);
            float crest = 0.7f + 0.3f * vk_sin(f * VK_TAU + v * 3.0f);
            /* hem: catenary sag with slow ripple */
            float hem = 0.78f + 0.10f * (u - 0.5f) * (u - 0.5f) * 4.0f
                      + 0.06f * vk_sin(u * 9.0f + t * 0.0025f) + 0.03f * vk_sin(u * 21.0f - t * 0.004f);
            float body = vk_sstep(hem + 0.05f, hem - 0.10f, v) * vk_sstep(0.0f, 0.06f, v);
            float m = rib * body * crest;
            float ci = base + f * 1200.0f + v * 2400.0f + t * 0.7f;
            float cj = ci + 2200.0f + vk_absf(px) * 900.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, 0.5f + 0.5f * vk_sin(v * 6.0f - t * 0.003f + px), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
