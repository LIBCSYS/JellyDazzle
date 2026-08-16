/* pattern_291 — MOSS CARPET (field): a cushion of moss — soft rounded
 * hummocks lit from above, dark crevices winding between them, dew glinting
 * on the crowns; the carpet swells and settles.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_291(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.001f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 4.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 6.0f;
            /* hummocks: worley-ish via 3x3 site distance */
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float d1 = 9.0f, id = 0.0f;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float sx = cx + 0.2f + 0.6f * vk_h2(cx, cy, seed), sy = cy + 0.2f + 0.6f * vk_h2(cx, cy, seed ^ 1u);
                float dx = u - sx, dy = v - sy, d = dx * dx + dy * dy;
                if (d < d1) { d1 = d; id = vk_h2(cx, cy, seed ^ 2u); }
            }
            float r = sqrtf(d1);
            float hgt = (1.0f - vk_sstep(0.0f, 0.75f, r)) * (0.8f + 0.2f * vk_sin(t * 0.003f + id * 9.0f));
            float fine = vk_noise3(u * 5.0f, v * 5.0f, tz, seed ^ 7u);
            float m = vk_sstep(0.12f, 0.55f, hgt + 0.15f * (fine - 0.5f));
            float dew = vk_sstep(0.85f, 0.95f, fine) * vk_sstep(0.5f, 0.9f, hgt);
            m = m * (0.7f + 0.3f * fine) + dew * 0.5f;
            m = m > 1.0f ? 1.0f : m;
            float ci = base + id * 1200.0f + hgt * 1500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, fine, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
