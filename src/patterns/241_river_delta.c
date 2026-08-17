/* pattern_241 — RIVER DELTA (field): braided channels seen from above,
 * splitting and rejoining as they fan toward the sea, water glowing, the
 * sandbars between them black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_241(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.0012f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw - 0.5f;
            /* channels: ridged noise stretched along the flow (down the frame),
             * fanning: horizontal scale grows with v */
            float fan = 0.35f + v;
            float su = u / fan * 8.0f, sv = v * 3.0f;
            float m = 0.0f;
            for (int o = 0; o < 3; o++) {
                float sc = 1.0f + o * 0.9f;
                float n = vk_noise3(su * sc + o * 3.0f, sv * sc * 0.6f - tz * 0.4f, tz, seed + o * 33u);
                float rid = 1.0f - vk_absf(n - 0.5f) * (5.0f + o * 2.0f);
                if (rid > m) m = rid;
            }
            m = m < 0.0f ? 0.0f : m;
            m = vk_sstep(0.25f, 0.85f, m);
            /* the delta only occupies a fan; sea at the bottom glows dim */
            float inside = vk_sstep(0.62f, 0.5f, vk_absf(u) / fan) * vk_sstep(0.0f, 0.08f, v);
            float sea = vk_sstep(0.85f, 1.05f, v) * 0.5f;
            m = m * inside;
            m = m > sea ? m : sea;
            float glint = 0.7f + 0.3f * vk_sin(v * 40.0f + u * 10.0f - t * 0.01f);
            float ci = base + v * 2000.0f + vk_absf(u) * 1500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, glint, m * glint));
        }
    }
    vk_blit(&cv, fb, w, h);
}
