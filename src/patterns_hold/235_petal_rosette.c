/* pattern_235 — PETAL ROSETTE (field): rings of overlapping petals open
 * outward from the centre in 6-fold symmetry, gaps between the petals black,
 * the flower breathing.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_235(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const int folds = 6 + 2 * (int)(vk_seedf(seed, 1) * 2.0f);   /* 6, 8 */
    const float rot = t * 0.0005f;
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u) + rot;
            float m = 0.0f, ci = base;
            for (int k = 0; k < 5; k++) {
                float rr = 0.14f + 0.22f * k + 0.02f * vk_sin(t * 0.004f + k * 1.3f);   /* ring radius */
                float a = ang * folds / VK_TAU + (k & 1 ? 0.5f : 0.0f);
                float fa = (vk_fract(a) - 0.5f) * (VK_TAU / folds);
                /* petal: ellipse pointing outward centred at rr */
                float px = r * vk_cos(fa) - rr, py = r * vk_sin(fa);
                float pl = 0.15f + 0.02f * k, pw = 0.075f + 0.012f * k;
                float d = px * px / (pl * pl) + py * py / (pw * pw);
                float pet = vk_sstep(1.0f, 0.75f, d);
                float vein = 0.75f + 0.25f * vk_sin(py / pw * 9.0f + px * 6.0f);
                float grad = 0.55f + 0.45f * (1.0f - d);
                float val = pet * vein * grad;
                if (val > m) { m = val; ci = base + k * 600.0f + d * 1200.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1700.0f, 0.5f + 0.5f * vk_sin(r * 10.0f - t * 0.003f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
