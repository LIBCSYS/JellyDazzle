/* pattern_215 — CORAL FAN (field): a sea fan (gorgonian) — a dense lattice
 * of forking branches spreading upward from a holdfast, swaying in the
 * current; water black between the branches.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_215(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    for (int y = 0; y < sh; y++) {
        float v = 1.15f - (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.5f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(u, v);
            ang += 0.06f * vk_sin(r * 2.0f + t * 0.0025f) * r + 0.02f * vk_sin(r * 9.0f - t * 0.004f);
            /* forking branches: angular frequency doubles every band of r,
             * each band a soft, wobbling ribbon */
            float m = 0.0f, lvl = 1.0f;
            for (int o = 0; o < 5; o++) {
                float nb = 5.0f * lvl;
                float wob = 0.12f * vk_noise2(r * 4.0f, o * 7.0f + ang * nb * 0.2f, seed);
                float f = vk_absf(vk_fract(ang * nb / VK_TAU + 0.5f + wob) - 0.5f);
                float thick = 0.27f - 0.02f * o;
                float br = vk_sstep(thick, thick * 0.3f, f);
                float band = vk_sstep(0.22f * o - 0.10f, 0.22f * o + 0.05f, r) * vk_sstep(0.22f * o + 0.55f, 0.22f * o + 0.35f, r);
                float val = br * band;
                if (val > m) m = val;
                lvl *= 2.0f;
            }
            /* mesh: cross-links between branches */
            float ring = vk_sstep(0.55f, 0.95f, 0.5f + 0.5f * vk_sin(r * 42.0f - t * 0.003f + ang * 2.0f));
            float rk = ring * 0.7f * vk_sstep(0.25f, 0.45f, r);
            if (rk > m) m = rk;
            float fan = vk_sstep(1.55f, 1.15f, r) * vk_sstep(1.5f, 1.0f, vk_absf(ang) * 1.2f);
            m *= fan;
            float ci = base + r * 2600.0f + vk_absf(ang) * 800.0f + t * 0.6f;
            float cj = ci + 1700.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, 0.5f + 0.5f * vk_sin(r * 8.0f + t * 0.003f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
