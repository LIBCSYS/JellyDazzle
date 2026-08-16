/* pattern_272 — HEX BOKEH (field): out-of-focus lights — soft hexagonal
 * bokeh discs of many sizes drifting slowly, overlapping into brighter
 * lobes, black between them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NBK 40
void pattern_272(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float bx[NBK], by[NBK], br[NBK], bc[NBK];
    for (int i = 0; i < NBK; i++) {
        float ph = vk_seedf(seed, i * 4 + 1) * VK_TAU;
        bx[i] = vk_seedf(seed, i * 4 + 2) * 1.333f + 0.06f * vk_sin(t * 0.0008f + ph);
        by[i] = vk_fract(vk_seedf(seed, i * 4 + 3) + t * 0.00006f * (1.0f + vk_seedf(seed, i))) * 1.3f - 0.15f;
        br[i] = 0.06f + 0.10f * vk_seedf(seed, i * 4 + 4);
        bc[i] = vk_seedf(seed, i) * 2600.0f;
    }
    /* accumulate into float buffer for additive glow */
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            uint32_t col = 0xFF000000u;
            for (int i = 0; i < NBK; i++) {
                float dx = u - bx[i], dy = v - by[i];
                if (vk_absf(dx) > br[i] || vk_absf(dy) > br[i]) continue;
                /* hexagonal distance */
                float hd = vk_absf(dx) * 0.8660254f + vk_absf(dy) * 0.5f;
                if (vk_absf(dy) > hd) hd = vk_absf(dy);
                float d = hd / br[i];
                if (d > 1.0f) continue;
                float m = vk_sstep(1.0f, 0.85f, d) * (0.45f + 0.35f * d) * (0.6f + 0.4f * vk_sin(t * 0.003f + i));
                float ci = base + bc[i] + d * 500.0f + t * 0.5f;
                col = vk_add(col, vk_pc2(pal, ci, ci + 1400.0f, d, m * 0.7f));
            }
            vk_putp(row + x * 3, col);
        }
    }
    vk_blit(&cv, fb, w, h);
}
