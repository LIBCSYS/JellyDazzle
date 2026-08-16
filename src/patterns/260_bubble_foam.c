/* pattern_260 — BUBBLE FOAM (field): tightly packed soap bubbles of many
 * sizes, each with a bright rim and a highlight, black in the gaps between
 * them; the mass drifts upward slowly.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_260(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            uint32_t col = 0xFF000000u;
            /* two size layers of jittered-grid bubbles */
            for (int L = 0; L < 2; L++) {
                float cells = L ? 11.0f : 6.0f;
                float vv = v * cells + t * 0.0004f * (L + 1) * cells * 0.5f;
                float uu = u * cells;
                int iu = (int)floorf(uu), iv = (int)floorf(vv);
                for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                    int cx = iu + i, cy = iv + j;
                    float h1 = vk_h2(cx, cy, seed + L * 77u), h2 = vk_h2(cx, cy, seed ^ (3u + L));
                    if (h2 < 0.2f) continue;
                    float sx = cx + 0.25f + 0.5f * h1 + 0.05f * vk_sin(t * 0.003f + h1 * 20.0f);
                    float sy = cy + 0.25f + 0.5f * h2;
                    float rad = 0.30f + 0.20f * vk_h2(cx, cy, seed ^ 9u);
                    float dx = uu - sx, dy = vv - sy;
                    float d = sqrtf(dx * dx + dy * dy) / rad;
                    if (d > 1.0f) continue;
                    float rim = vk_sstep(0.70f, 0.96f, d) * vk_sstep(1.0f, 0.97f, d);
                    float film = 0.25f + 0.2f * vk_sin(d * 9.0f + t * 0.004f + h1 * 6.0f);
                    float hi = vk_sstep(0.5f, 0.0f, sqrtf((dx / rad + 0.35f) * (dx / rad + 0.35f) + (dy / rad + 0.35f) * (dy / rad + 0.35f)) * 2.0f) * 0.7f;
                    float m = rim > film ? rim : film;
                    m = m > hi ? m : hi;
                    float ci = base + h1 * 2000.0f + d * 900.0f + t * 0.5f;
                    col = vk_max(col, vk_pc2(pal, ci, ci + 1500.0f, d, m));
                }
            }
            vk_putp(row + x * 3, col);
        }
    }
    vk_blit(&cv, fb, w, h);
}
