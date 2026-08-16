/* pattern_228 — ROPE NET (field): a knotted fishing net hangs and sags,
 * ropes twisted, knots at every crossing; the water behind is black.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_228(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 3.0f, 4.5f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            /* sag: the net bellies with a slow swell */
            float sx = u + 0.12f * vk_sin(v * 2.0f + t * 0.0022f) * (1.0f - u * u * 0.3f);
            float sy = v + 0.10f * vk_sin(u * 2.5f - t * 0.0017f) + 0.05f * u * u;
            float a = (sx + sy) * n, b = (sx - sy) * n;
            float fa = vk_absf(vk_fract(a) - 0.5f), fbb = vk_absf(vk_fract(b) - 0.5f);
            float da = fa, db = fbb;
            float rw = 0.17f;
            float ra = vk_sstep(rw, rw * 0.3f, da), rb = vk_sstep(rw, rw * 0.3f, db);
            /* twist shading along the rope */
            float twa = 0.7f + 0.3f * vk_sin(b * 24.0f + da * 40.0f), twb = 0.7f + 0.3f * vk_sin(a * 24.0f + db * 40.0f);
            float rope = ra * twa > rb * twb ? ra * twa : rb * twb;
            /* knots at crossings */
            float kd = sqrtf(fa * fa + fbb * fbb);
            float knot = vk_sstep(0.26f, 0.15f, kd);
            float m = rope > knot ? rope : knot;
            m *= 0.9f + 0.1f * vk_sin(sx * 3.0f + sy * 2.0f + t * 0.003f);
            float ci = base + (sx + sy) * 900.0f + knot * 1200.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1700.0f, ra > rb ? twa : twb, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
