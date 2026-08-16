/* pattern_256 — WING VEINS (field): a dragonfly wing — a translucent
 * membrane cut into cells by dark veins, cells catching iridescent light in
 * turn, some cells clear (black).  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_256(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = 11.0f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells * 0.6f;      /* cells elongated along x */
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float d1 = 9.0f, d2 = 9.0f, id = 0.0f;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float sx = cx + 0.2f + 0.6f * vk_h2(cx, cy, seed);
                float sy = cy + 0.2f + 0.6f * vk_h2(cx, cy, seed ^ 1u);
                float dx = (u - sx) * 1.6f, dy = v - sy;
                float d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; id = vk_h2(cx, cy, seed ^ 2u); }
                else if (d < d2) d2 = d;
            }
            float vein = vk_sstep(0.02f, 0.09f, d2 - d1);
            /* iridescence sweeps across the wing */
            float sweep = 0.5f + 0.5f * vk_sin((u * 0.6f + v * 0.3f) * 1.2f - t * 0.004f + id * 3.0f);
            float clear = vk_sstep(0.30f, 0.50f, id * 0.5f + sweep * 0.5f);
            float m = vein * clear * (0.5f + 0.5f * sweep);
            /* vein highlight thin line */
            float vl = vk_sstep(0.09f, 0.11f, d2 - d1) * 0.0f + vk_sstep(0.03f, 0.0f, vk_absf(d2 - d1 - 0.05f)) * 0.5f;
            m = m > vl ? m : vl;
            float ci = base + id * 1500.0f + sweep * 1800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, sweep, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
