/* pattern_307 — LILY PADS (field): lily pads floating on black water,
 * round with a notch to the stem, veined from the centre, drifting and
 * turning slowly, overlapping.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NLP 16
void pattern_307(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float px[NLP], py[NLP], pr[NLP], pa[NLP];
    for (int i = 0; i < NLP; i++) {
        px[i] = vk_seedf(seed, i * 4 + 1) * 1.333f + 0.04f * vk_sin(t * 0.0009f + i);
        py[i] = vk_seedf(seed, i * 4 + 2) + 0.03f * vk_cos(t * 0.0007f + i * 2.0f);
        pr[i] = 0.10f + 0.12f * vk_seedf(seed, i * 4 + 3);
        pa[i] = vk_seedf(seed, i * 4 + 4) * VK_TAU + t * 0.0006f * (i & 1 ? 1 : -1);
    }
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float m = 0.0f, ci = base;
            for (int i = 0; i < NLP; i++) {
                float dx = u - px[i], dy = v - py[i];
                if (vk_absf(dx) > pr[i] || vk_absf(dy) > pr[i]) continue;
                float d = sqrtf(dx * dx + dy * dy) / pr[i];
                if (d > 1.0f) continue;
                float ang = atan2f(dy, dx) - pa[i];
                float na = vk_absf(vk_fract(ang / VK_TAU + 0.5f) - 0.5f) * VK_TAU;   /* 0 at notch axis */
                float notch = vk_sstep(0.25f, 0.45f, na + (1.0f - d) * 0.5f);            /* wedge missing */
                float pad = vk_sstep(1.0f, 0.92f, d) * notch;
                float vein = 0.8f + 0.2f * vk_sstep(0.85f, 1.0f, 0.5f + 0.5f * vk_cos(ang * 9.0f));
                float shade = 0.55f + 0.45f * (1.0f - d * d);
                float val = pad * vein * shade;
                if (val > m) { m = val; ci = base + i * 200.0f + d * 1200.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(u * 4.0f + v * 3.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
