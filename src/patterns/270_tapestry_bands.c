/* pattern_270 — TAPESTRY BANDS (field): horizontal bands of woven motif —
 * diamonds, zigzags, dots — each band its own colour and rhythm, dark
 * selvedge between bands, the cloth swaying.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_270(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float nb = 7.0f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f + 0.03f * vk_sin(v * 6.0f + t * 0.002f);
            float bv = v * nb + 0.02f * vk_sin(u * 5.0f + t * 0.0015f);
            int ib = (int)floorf(bv);
            float f = vk_fract(bv);
            float band = vk_sstep(0.10f, 0.22f, f) * vk_sstep(0.90f, 0.78f, f);
            int kind = (int)(vk_h2(ib, 0, seed) * 3.0f);
            float k = 6.0f + floorf(vk_h2(ib, 1, seed) * 6.0f);
            float pu = u * k + t * 0.001f * (ib & 1 ? 1 : -1);
            float motif;
            if (kind == 0) {          /* diamonds */
                float a = vk_absf(vk_fract(pu) - 0.5f) + vk_absf(f - 0.5f) * 1.2f;
                motif = vk_sstep(0.42f, 0.30f, a);
            } else if (kind == 1) {   /* zigzag */
                float z = vk_absf(vk_tri(pu) - f);
                motif = vk_sstep(0.16f, 0.06f, z);
            } else {                  /* dots */
                float dx = vk_fract(pu) - 0.5f, dy = (f - 0.5f) * 1.2f;
                motif = vk_sstep(0.30f, 0.20f, sqrtf(dx * dx + dy * dy));
            }
            float m = band * (0.22f + 0.78f * motif);
            float ci = base + vk_h2(ib, 2, seed) * 2400.0f + motif * 1200.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, motif, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
