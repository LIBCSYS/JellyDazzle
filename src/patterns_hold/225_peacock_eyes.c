/* pattern_225 — PEACOCK EYES (field): a fan of tail feathers, each carrying
 * an eye-spot of concentric colour, barbs shimmering; black between the
 * plumes.  Mirror symmetry.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_225(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const int np = 9;
    for (int y = 0; y < sh; y++) {
        float v = 1.25f - (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.7f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(u, v);
            float sway = 0.03f * vk_sin(r * 3.0f + t * 0.002f);
            float pa = (ang + sway) * (float)np / VK_TAU * 2.0f + 0.5f;    /* np plumes over half-turn */
            float f = vk_fract(pa) - 0.5f;
            float pid = floorf(pa);
            float w2 = 0.36f * vk_sstep(0.1f, 0.5f, r) * vk_sstep(1.5f, 1.1f, r);
            float body = vk_sstep(w2, w2 * 0.2f, vk_absf(f));
            float barb = 0.7f + 0.3f * vk_sin(f * 50.0f + r * 8.0f + t * 0.006f);
            /* eye at r ~ 0.95 on the plume axis */
            float ex = f * r * 1.2f, ey = r - 0.95f - 0.03f * vk_sin(t * 0.004f + pid);
            float ed = sqrtf(ex * ex * 4.0f + ey * ey * 2.5f);
            float eye = vk_sstep(0.20f, 0.17f, ed);
            float m = body * barb * vk_sstep(0.1f, 0.3f, r) * vk_sstep(1.35f, 1.15f, vk_absf(ang));
            float ci = base + r * 1400.0f + vk_absf(f) * 2600.0f + t * 0.5f;
            if (eye > 0.0f) { m = m * (1.0f - eye) + eye; ci = base + 2800.0f + ed * 12000.0f + t * 0.5f; }
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, barb, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
