/* pattern_219 — LACE DOILY (field): a crocheted lace round — rings of
 * scalloped loops in 12-fold symmetry, open holes between the stitches,
 * turning very slowly.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_219(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float rot = t * 0.0004f;
    const int folds = 12;
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u) + rot;
            float m = 0.0f;
            /* concentric rings of loops; each ring has folds*k loops */
            for (int k = 1; k <= 6; k++) {
                float rr = k * 0.155f;
                float n = (float)(folds * (k < 3 ? 1 : k < 5 ? 2 : 3));
                float a = ang * n / VK_TAU + (k & 1 ? 0.5f : 0.0f);
                float fa = (vk_fract(a) - 0.5f) * (VK_TAU / n);
                /* loop = scallop: a small circle centred at (rr, angle) */
                float lx = r * vk_cos(fa) - rr, ly = r * vk_sin(fa);
                float loopr = 0.075f - 0.004f * k;
                float d = sqrtf(lx * lx + ly * ly);
                float ring = vk_sstep(0.024f, 0.008f, vk_absf(d - loopr * (1.0f + 0.05f * vk_sin(t * 0.005f + k))));
                if (ring > m) m = ring;
                /* thin radial thread between rings */
                float th = vk_sstep(0.012f, 0.004f, vk_absf(r * vk_sin(fa))) * vk_sstep(rr - 0.03f, rr, r) * vk_sstep(rr + 0.16f, rr + 0.12f, r);
                if (th * 0.7f > m) m = th * 0.7f;
            }
            float ci = base + r * 3000.0f + t * 0.6f;
            float cj = ci + 1500.0f + ang * 200.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, 0.5f + 0.5f * vk_sin(r * 12.0f - t * 0.003f), m * (0.8f + 0.2f * vk_sin(ang * 12.0f))));
        }
    }
    vk_blit(&cv, fb, w, h);
}
