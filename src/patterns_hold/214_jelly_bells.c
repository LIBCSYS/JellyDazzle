/* pattern_214 — JELLY BELLS (field): a bloom of jellyfish, translucent bells
 * pulsing slowly with trailing tentacles, black water between.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NJ 13
void pattern_214(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float jx[NJ], jy[NJ], jr[NJ], jp[NJ], jf[NJ];
    for (int i = 0; i < NJ; i++) {
        float ph = vk_seedf(seed, i * 3 + 1) * VK_TAU;
        /* TEMPORAL REVIEW 2.4.0 (docs/review/04_pattern_temporal.md, F-214):
         * jy wraps through vk_fract, and a bell at jy=-0.2 still trails
         * tentacles ~1.4 screen-heights down — ON screen — so the wrap
         * teleported a visible jelly bottom-to-top in one frame (delta 2.2
         * pop on a 0.78 median).  Fade each jelly over the last/first 4% of
         * its travel so it dissolves before it jumps. */
        float q = vk_fract(vk_seedf(seed, i * 3 + 2) - t * 0.00012f * (0.6f + 0.6f * vk_seedf(seed, i)));
        jx[i] = 0.5f + 0.5f * vk_sin(t * 0.0009f + ph * 2.0f);
        jy[i] = q * 1.4f - 0.2f;
        jf[i] = vk_sstep(0.0f, 0.04f, q) * vk_sstep(1.0f, 0.96f, q);
        jr[i] = 0.28f + 0.16f * vk_seedf(seed, i * 3 + 3);
        jp[i] = t * 0.006f + ph;
    }
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            uint32_t col = 0xFF000000u;
            for (int i = 0; i < NJ; i++) {
                float dx = u - jx[i] * 1.333f, dy = v - jy[i];
                float pulse = 1.0f + 0.10f * vk_sin(jp[i]);
                float rx = jr[i] * pulse, ry = jr[i] * (1.05f - 0.10f * vk_sin(jp[i])) * 0.8f;
                float m = 0.0f, ci = base + i * 700.0f;
                float dyb = dy < 0.0f ? dy : dy * 2.2f;   /* bell: dome above, shallow skirt below */
                float d = sqrtf(dx * dx / (rx * rx) + dyb * dyb / (ry * ry));
                if (d < 1.0f) {
                    float rim = vk_sstep(1.0f, 0.90f, d);
                    float rib = 0.80f + 0.20f * vk_sin(atan2f(dy, dx) * 14.0f + d * 3.0f);
                    float dome = 0.45f + 0.55f * d * d;      /* thin centre, dense rim */
                    m = rim * dome * rib;
                    ci += d * 1500.0f;
                } else if (dy > 0.0f && dy < ry * 4.5f) {          /* tentacles */
                    float ax = dx / rx;
                    if (vk_absf(ax) < 0.9f) {
                        float wig = 0.08f * vk_sin(dy * 30.0f - t * 0.02f + ax * 4.0f);
                        float ten = 0.5f + 0.5f * vk_sin((ax + wig) * 28.0f);
                        m = vk_sstep(0.35f, 0.9f, ten) * vk_sstep(ry * 4.5f, 0.0f, dy) * 0.85f
                          * vk_sstep(0.9f, 0.5f, vk_absf(ax));
                        ci += 2200.0f + dy * 3000.0f;
                    }
                }
                if (m > 0.0f) col = vk_max(col, vk_pc2(pal, ci + t * 0.5f, ci + 1800.0f, 0.5f + 0.5f * vk_sin(jp[i] * 0.5f), m * jf[i]));
            }
            vk_putp(row + x * 3, col);
        }
    }
    vk_blit(&cv, fb, w, h);
}
