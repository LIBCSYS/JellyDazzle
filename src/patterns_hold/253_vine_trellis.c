/* pattern_253 — VINE TRELLIS (field): a diagonal trellis with vines wound
 * along its bars, leaves budding along the vines and swaying; the sky
 * through the trellis is black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_253(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 3.0f, 4.2f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float a = (u + v) * n, b = (u - v) * n;
            float fa = vk_fract(a) - 0.5f, fbb = vk_fract(b) - 0.5f;
            /* bars */
            float bar = vk_sstep(0.13f, 0.05f, vk_absf(fa)) > vk_sstep(0.13f, 0.05f, vk_absf(fbb)) ? vk_sstep(0.13f, 0.05f, vk_absf(fa)) : vk_sstep(0.13f, 0.05f, vk_absf(fbb));
            /* vine: sinusoid winding around bar a */
            float wind = 0.09f * vk_sin(b * 6.0f + t * 0.002f);
            float vine = vk_sstep(0.08f, 0.02f, vk_absf(fa - wind));
            float wind2 = 0.09f * vk_sin(a * 6.0f - t * 0.0017f);
            float vine2 = vk_sstep(0.08f, 0.02f, vk_absf(fbb - wind2));
            /* leaves: teardrops at intervals along the vines */
            float lp = vk_fract(b * 1.5f + 0.25f), lq = vk_fract(a * 1.5f + 0.75f);
            float lx = (lp - 0.5f) / 1.5f / n * n, ly = fa - wind - 0.12f * vk_sin(t * 0.003f + floorf(b * 1.5f));
            float leaf = vk_sstep(0.28f, 0.10f, sqrtf(lx * lx * 2.5f + ly * ly * 5.0f));
            float lx2 = (lq - 0.5f), ly2 = fbb - wind2 + 0.12f * vk_sin(t * 0.0025f + floorf(a * 1.5f));
            float leaf2 = vk_sstep(0.28f, 0.10f, sqrtf(lx2 * lx2 * 2.5f + ly2 * ly2 * 5.0f));
            float m = bar * 0.6f;
            m = m > vine * 0.8f ? m : vine * 0.8f;
            m = m > vine2 * 0.8f ? m : vine2 * 0.8f;
            float lf = leaf > leaf2 ? leaf : leaf2;
            float ci = base + t * 0.5f + (lf > m ? 2000.0f + lf * 1200.0f : bar > 0.3f ? 0.0f : 900.0f);
            m = m > lf ? m : lf;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, 0.5f + 0.5f * vk_sin((a + b) * 2.0f + t * 0.002f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
