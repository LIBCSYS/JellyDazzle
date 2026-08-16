/* pattern_304 — TWILL WEAVE (field): a twill fabric — diagonal ribs made of
 * staggered floats, some floats missing so slots of black run through the
 * cloth; the fabric flexes.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_304(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 12.0f, 18.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float a = (u + 0.05f * vk_sin(v * 3.0f + t * 0.002f)) * n, b = (v + 0.05f * vk_sin(u * 2.0f - t * 0.0015f)) * n * 1.4f;
            int ia = (int)floorf(a), ib = (int)floorf(b);
            float fa = vk_fract(a), fbb = vk_fract(b);
            /* twill: float pattern shifts by one each row (2/2 twill) */
            int k = ((ia - ib) % 4 + 4) % 4;
            int on = k < 2;
            /* dropped floats create slots: hash on the diagonal index */
            float diag = vk_h2((ia - ib + 400) / 4, 0, seed);
            float slot = vk_sstep(0.25f, 0.35f, diag + 0.1f * vk_sin(t * 0.002f + diag * 20.0f));
            float sh_a = vk_sstep(0.0f, 0.15f, fa) * vk_sstep(1.0f, 0.85f, fa);
            float sh_b = vk_sstep(0.0f, 0.2f, fbb) * vk_sstep(1.0f, 0.8f, fbb);
            float shade = 0.6f + 0.4f * vk_sin(fbb * 3.14159f);
            float m = on ? sh_a * shade * slot : sh_b * 0.5f * shade;
            float ci = base + (on ? 0.0f : 1800.0f) + diag * 900.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, shade, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
