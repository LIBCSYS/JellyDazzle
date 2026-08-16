/* pattern_237 — DEW WEB (field): an orb web strung across the frame, radial
 * threads and a spiral, dew beads glinting along the silk, sagging with a
 * slow breeze; the air behind is black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_237(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const int nr = 18 + (int)(vk_seedf(seed, 1) * 8.0f);
    const float cx = 0.5f + 0.15f * vk_sin(t * 0.0007f), cy = 0.5f + 0.1f * vk_cos(t * 0.0005f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh - cy;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - cx) * 1.333f;
            float r = sqrtf(u * u + v * v) + 1e-4f;
            float ang = atan2f(v, u);
            /* radials */
            float fa = vk_absf(vk_fract(ang * nr / VK_TAU + 0.5f) - 0.5f) * (VK_TAU / nr) * r;
            float rad = vk_sstep(0.010f, 0.002f, fa);
            /* spiral with sag between radials */
            float sector = vk_fract(ang * nr / VK_TAU + 0.5f) - 0.5f;
            float sag = 0.012f * (1.0f - 4.0f * sector * sector) * (1.0f + 0.3f * vk_sin(t * 0.004f + r * 10.0f));
            float sp = (r - sag) * 26.0f - ang / VK_TAU + t * 0.0005f;
            float fs = vk_absf(vk_fract(sp) - 0.5f) / 26.0f;
            float spi = vk_sstep(0.008f, 0.002f, fs) * vk_sstep(0.05f, 0.1f, r);
            /* dew beads: on spiral, periodic along it */
            float bead = vk_sstep(0.85f, 0.97f, 0.5f + 0.5f * vk_sin(ang * 60.0f + sp * 3.0f)) * vk_sstep(0.016f, 0.006f, fs);
            float m = rad > spi ? rad : spi;
            m = m * 0.75f + bead;
            m = m > 1.0f ? 1.0f : m;
            m *= vk_sstep(0.9f, 0.6f, r);
            float ci = base + r * 3000.0f + bead * 1500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, bead, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
