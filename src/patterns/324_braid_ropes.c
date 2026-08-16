/* pattern_324 — BRAID ROPES (field): three-strand braids running across the
 * frame in rows, strands passing over and under with cylinder shading, black
 * between the braids and in the gaps of the plait.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_324(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float rows = 4.0f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * rows;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            int ir = (int)floorf(v);
            float fv = (vk_fract(v) - 0.5f) * 2.0f;                 /* -1..1 across the row */
            float p = u * 7.0f + t * 0.002f * (ir & 1 ? 1.0f : -1.0f) + vk_h2(ir, 0, seed) * 6.0f + 0.05f * vk_sin(v * 2.0f + t * 0.002f);
            float m = 0.0f, ci = base;
            for (int s = 0; s < 3; s++) {
                /* strand centre: sinusoid with phase offset; z-order from cos */
                float ph = p + s * (VK_TAU / 3.0f);
                float c = 0.55f * vk_sin(ph);
                float z = vk_cos(ph);                                 /* +1 = front */
                float d = vk_absf(fv - c) / 0.28f;
                if (d > 1.0f) continue;
                float body = vk_sstep(1.0f, 0.8f, d) * sqrtf(1.0f - d * d * 0.9f);
                float depth = 0.55f + 0.45f * (z * 0.5f + 0.5f);
                float twist = 0.85f + 0.15f * vk_sin(p * 12.0f + d * 3.0f);
                float val = body * depth * twist;
                /* front strand wins by z */
                float pri = val * (1.0f + 0.5f * z);
                if (pri > m) { m = pri > 1.0f ? 1.0f : pri; ci = base + s * 900.0f + vk_h2(ir, 1, seed) * 1200.0f + d * 500.0f; }
            }
            m *= vk_sstep(1.0f, 0.9f, vk_absf(fv));
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(u * 3.0f + t * 0.002f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
