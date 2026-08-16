/* pattern_333 — TULLE GATHER (field): tulle gathered to a point above the
 * frame, falling in radiating pleats that soften and spread toward the hem;
 * black between the pleats and below the hem.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_333(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float gx = 0.667f + 0.15f * vk_sin(t * 0.0008f), gy = -0.35f;
    const float np = vk_seedr(seed, 1, 30.0f, 44.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float dx = u - gx, dy = v - gy;
            float r = sqrtf(dx * dx + dy * dy);
            float ang = atan2f(dx, dy);
            float sway = 0.04f * vk_sin(r * 4.0f + t * 0.003f) + 0.02f * vk_sin(ang * 7.0f - t * 0.002f);
            float p = (ang + sway) * np / VK_TAU * 2.0f;
            float f = vk_fract(p);
            /* pleats: soft bands, spreading (softer) with distance */
            float soft = 0.10f + 0.25f * vk_sstep(0.3f, 1.4f, r);
            float pleat = vk_sstep(0.0f, soft, f) * vk_sstep(0.75f, 0.75f - soft, f);
            float net = 0.85f + 0.15f * vk_sin(u * 120.0f) * vk_sin(v * 120.0f);
            float hem = 0.98f + 0.05f * vk_sin(ang * 9.0f + t * 0.002f);
            float m = pleat * net * vk_sstep(hem, hem - 0.15f, v) * (0.5f + 0.5f * vk_sstep(0.3f, 1.2f, r));
            float ci = base + r * 1800.0f + f * 600.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
