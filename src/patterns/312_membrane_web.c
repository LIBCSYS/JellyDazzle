/* pattern_312 — MEMBRANE WEB (field): a reticulated membrane — thick soft
 * strands joined in a net, the strands glowing and the big cells between
 * them black, the net stretching and relaxing.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_312(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = vk_seedr(seed, 1, 4.0f, 6.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float d1 = 9.0f, d2 = 9.0f, d3 = 9.0f;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float ph = vk_h2(cx, cy, seed) * VK_TAU;
                float sx = cx + 0.5f + 0.35f * vk_sin(t * 0.002f + ph), sy = cy + 0.5f + 0.35f * vk_cos(t * 0.0017f + ph * 1.4f);
                float dx = u - sx, dy = v - sy, d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d3 = d2; d2 = d1; d1 = d; } else if (d < d2) { d3 = d2; d2 = d; } else if (d < d3) d3 = d;
            }
            float e = d2 - d1;
            float strand = vk_sstep(0.30f, 0.05f, e);
            float node = vk_sstep(0.42f, 0.12f, d3 - d1);        /* junctions thicker */
            float m = strand * 0.8f + node * 0.4f;
            m = m > 1.0f ? 1.0f : m;
            float glow = 0.75f + 0.25f * vk_sin(d1 * 8.0f - t * 0.005f);
            float ci = base + e * 2500.0f + d1 * 900.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, node, m * glow));
        }
    }
    vk_blit(&cv, fb, w, h);
}
