/* pattern_273 — RIBBON STREAMS (field): wide silk ribbons stream across the
 * frame on slow sine paths, twisting so they narrow and widen, overlapping
 * translucently; black air between them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NRB 7
void pattern_273(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float y0[NRB], amp[NRB], frq[NRB], ph[NRB], wd[NRB], sp[NRB];
    for (int i = 0; i < NRB; i++) {
        y0[i] = (i + 0.5f) / NRB + 0.08f * (vk_seedf(seed, i) - 0.5f);
        amp[i] = 0.10f + 0.12f * vk_seedf(seed, i + 10);
        frq[i] = 2.0f + 3.0f * vk_seedf(seed, i + 20);
        ph[i] = vk_seedf(seed, i + 30) * VK_TAU;
        wd[i] = 0.05f + 0.05f * vk_seedf(seed, i + 40);
        sp[i] = (vk_seedf(seed, i + 50) - 0.5f) * 0.006f;
    }
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            uint32_t col = 0xFF000000u;
            for (int i = 0; i < NRB; i++) {
                float cy = y0[i] + amp[i] * vk_sin(u * frq[i] + ph[i] + t * sp[i]) + 0.03f * vk_sin(u * 9.0f - t * 0.003f + i);
                /* twist: apparent width follows a slow cosine along u */
                float tw = vk_cos(u * frq[i] * 0.7f + t * 0.002f + ph[i] * 2.0f);
                float half = wd[i] * (0.25f + 0.75f * vk_absf(tw));
                float d = vk_absf(v - cy);
                if (d > half) continue;
                float edge = vk_sstep(half, half * 0.6f, d);
                float face = 0.85f + 0.15f * tw;                     /* front / back of the ribbon */
                float sheen = 0.7f + 0.3f * vk_sin(d / half * 3.0f + u * 20.0f + t * 0.005f);
                float m = edge * face * sheen;
                float ci = base + i * 550.0f + (0.5f - 0.5f * tw) * 1500.0f + u * 600.0f + t * 0.5f;
                col = vk_max(col, vk_pc2(pal, ci, ci + 1400.0f, d / half, m));
            }
            vk_putp(row + x * 3, col);
        }
    }
    vk_blit(&cv, fb, w, h);
}
