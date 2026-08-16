/* pattern_292 — OP WAVES (field): an op-art grid of lines bulging as if a
 * sphere pressed up under it, the bulge wandering slowly; thick soft lines,
 * black between.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_292(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 8.0f, 12.0f);
    const float bx = 0.4f * vk_sin(t * 0.0009f), by = 0.3f * vk_cos(t * 0.0007f);
    const float bx2 = -0.5f * vk_sin(t * 0.0006f + 2.0f), by2 = 0.35f * vk_sin(t * 0.0011f + 1.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            /* bulge: radial displacement around two centres */
            float dx = u - bx, dy = v - by, r = sqrtf(dx * dx + dy * dy);
            float g = 0.35f * expf(-r * r * 3.0f);
            float dx2 = u - bx2, dy2 = v - by2, r2 = sqrtf(dx2 * dx2 + dy2 * dy2);
            float g2 = -0.25f * expf(-r2 * r2 * 4.0f);
            float uu = u + dx * (g + g2), vv = v + dy * (g + g2);
            float a = uu * n, b = vv * n;
            float fa = vk_absf(vk_fract(a) - 0.5f), fbb = vk_absf(vk_fract(b) - 0.5f);
            float la = vk_sstep(0.22f, 0.08f, fa), lb = vk_sstep(0.22f, 0.08f, fbb);
            float m = la > lb ? la : lb;
            float ci = base + (g + g2 + 0.3f) * 3000.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, la, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
