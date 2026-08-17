/* pattern_325 — MOTH WING (field): a pair of moth wings spread across the
 * frame, scaled in bands of colour with an eyespot on each, fringed edges
 * trembling; black beyond the wings.  Mirror symmetry.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_325(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float flap = 1.0f + 0.04f * vk_sin(t * 0.003f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = vk_absf((float)x / (float)sw - 0.5f) * 2.0f;
            /* forewing: ellipse tilted up-out; hindwing: rounder, lower */
            float fx = (u - 0.55f) * flap, fy = v + 0.25f - 0.35f * u;
            float fw = fx * fx / 0.30f + fy * fy / 0.10f;
            float hx = (u - 0.42f) * flap, hy = v - 0.32f + 0.15f * u;
            float hw = hx * hx / 0.16f + hy * hy / 0.11f;
            float fringe = 0.05f * vk_sin(atan2f(fy, fx) * 20.0f + t * 0.004f);
            float fore = vk_sstep(1.0f + fringe, 0.9f, fw) * vk_sstep(0.05f, 0.12f, u);
            float hind = vk_sstep(1.0f + fringe, 0.9f, hw) * vk_sstep(0.05f, 0.12f, u);
            /* bands: follow the wing shape (contours of fw / hw) */
            float band = fore > hind ? fw : hw;
            float bands = 0.5f + 0.5f * vk_sin(band * 7.0f - t * 0.003f);
            float scales = 0.85f + 0.15f * vk_noise2(u * 60.0f, v * 60.0f, seed);
            /* eyespot on forewing */
            float ex = fx - 0.12f, ey = fy + 0.05f;
            float ed = sqrtf(ex * ex + ey * ey * 2.5f);
            float eye = vk_sstep(0.16f, 0.14f, ed);
            float m = (fore > hind ? fore : hind) * (0.45f + 0.55f * vk_sstep(0.25f, 0.75f, bands)) * scales;
            float ci = base + band * 900.0f + bands * 1200.0f + t * 0.5f;
            if (eye > 0.0f) { m = m * (1.0f - eye) + eye * (0.7f + 0.3f * vk_sstep(0.05f, 0.0f, ed)); ci = base + 2800.0f + ed * 12000.0f + t * 0.5f; }
            /* body along the axis */
            float body = vk_sstep(0.07f, 0.03f, u) * vk_sstep(0.7f, 0.5f, vk_absf(v));
            m = m > body ? m : body;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, bands, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
