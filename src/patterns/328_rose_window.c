/* pattern_328 — ROSE WINDOW (field): a great rose window — twelve-fold
 * tracery of petals and roundels, panes lit in colour and some left dark,
 * light moving slowly across the glass.  12-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_328(uint32_t *fb, int w, int h, int frame, int sl,
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
            float r = sqrtf(u * u + v * v) * 0.82f;
            float ang = atan2f(v, u) + t * 0.0002f;
            /* petal ring: 12 petals between r=0.22 and 0.55; outer roundels ring at 0.68 */
            float a12 = vk_fract(ang * 12.0f / VK_TAU + 0.5f) - 0.5f;
            float pw = 0.5f * vk_sin(vk_clamp01((r - 0.20f) / 0.36f) * 3.14159f);
            float petal = vk_sstep(pw, pw - 0.10f, vk_absf(a12) * 2.0f) * vk_sstep(0.20f, 0.24f, r) * vk_sstep(0.57f, 0.53f, r);
            float a24 = vk_fract(ang * 24.0f / VK_TAU) - 0.5f;
            float rx = r * vk_cos(a24 * VK_TAU / 24.0f) - 0.68f, ry = r * vk_sin(a24 * VK_TAU / 24.0f);
            float roundel = vk_sstep(0.09f, 0.075f, sqrtf(rx * rx + ry * ry));
            float centre = vk_sstep(0.19f, 0.17f, r);
            float a6 = vk_absf(vk_fract(ang * 12.0f / VK_TAU) - 0.5f);
            float outer = vk_sstep(0.83f, 0.81f, r) * vk_sstep(0.59f, 0.61f, r) * vk_sstep(0.05f, 0.12f, a6) * (1.0f - roundel) * 0.7f;
            /* which panes lit */
            int pid = petal > 0.5f ? (int)floorf(ang * 12.0f / VK_TAU + 0.5f) : roundel > 0.5f ? 100 + (int)floorf(ang * 24.0f / VK_TAU) : centre > 0.5f ? 200 : 300 + (int)floorf(ang * 6.0f / VK_TAU);
            float lit = 0.5f + 0.5f * vk_sin(t * 0.002f + vk_h2(pid, 0, seed) * VK_TAU);
            float on = vk_sstep(0.12f, 0.42f, lit);
            float glass = petal > roundel ? petal : roundel; glass = glass > centre ? glass : centre; glass = glass > outer ? glass : outer;
            float streak = 0.75f + 0.25f * vk_sin(u * 12.0f + v * 9.0f + t * 0.003f);
            float m = glass * on * streak;
            /* tracery lines (bright leading) */
            float trace = vk_sstep(0.03f, 0.01f, vk_absf(r - 0.22f)) + vk_sstep(0.03f, 0.01f, vk_absf(r - 0.56f)) + vk_sstep(0.03f, 0.01f, vk_absf(r - 0.78f));
            trace = trace > 1.0f ? 1.0f : trace;
            m = m > trace * 0.5f ? m : trace * 0.5f;
            m *= vk_sstep(0.86f, 0.83f, r);
            float ci = base + vk_h2(pid, 1, seed) * 3200.0f + r * 500.0f + t * 0.4f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 900.0f, streak, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
