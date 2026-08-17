/* pattern_315 — CORAL MANDALA (field): three sea fans folded into a
 * three-fold mandala, forking branches meshing into a lattice toward the
 * rim, black water between the branches.  3-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_315(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u) + t * 0.0004f;
            /* fold to 3-fold mirror */
            float wa = VK_TAU / 6.0f;
            float fa = fmodf(ang, 2.0f * wa); if (fa < 0.0f) fa += 2.0f * wa;
            if (fa > wa) fa = 2.0f * wa - fa;
            fa += 0.06f * vk_sin(r * 3.0f + t * 0.002f) * r;
            float m = 0.0f, lvl = 1.0f;
            for (int o = 0; o < 4; o++) {
                float nb = 3.0f * lvl;
                float wob = 0.10f * vk_noise2(r * 5.0f, o * 7.0f + fa * nb * 0.3f, seed);
                float f = vk_absf(vk_fract(fa * nb / wa * 0.5f + 0.5f + wob) - 0.5f);
                float thick = 0.24f - 0.03f * o;
                float br = vk_sstep(thick, thick * 0.3f, f);
                float band = vk_sstep(0.25f * o - 0.05f, 0.25f * o + 0.08f, r) * vk_sstep(0.25f * o + 0.6f, 0.25f * o + 0.4f, r);
                float val = br * band;
                if (val > m) m = val;
                lvl *= 2.0f;
            }
            float ring = vk_sstep(0.6f, 0.95f, 0.5f + 0.5f * vk_sin(r * 36.0f - t * 0.003f)) * 0.7f * vk_sstep(0.3f, 0.5f, r);
            if (ring > m) m = ring;
            m *= vk_sstep(1.35f, 1.0f, r) * vk_sstep(0.0f, 0.08f, r);
            float ci = base + r * 2400.0f + fa * 1500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, 0.5f + 0.5f * vk_sin(r * 8.0f + t * 0.003f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
