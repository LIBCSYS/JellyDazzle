/* pattern_311 — AGATE SLICES (field): polished agate — irregular concentric
 * bands growing around several nuclei, meeting in seams, some bands
 * translucent black; the stone glows and dims.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_311(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = 2.5f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float d1 = 9.0f, id = 0.0f, ang = 0.0f;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float sx = cx + 0.2f + 0.6f * vk_h2(cx, cy, seed), sy = cy + 0.2f + 0.6f * vk_h2(cx, cy, seed ^ 1u);
                float dx = u - sx, dy = v - sy;
                float a = atan2f(dy, dx);
                /* irregular metric: radius wobbles with angle */
                float d = sqrtf(dx * dx + dy * dy) * (1.0f + 0.15f * vk_sin(a * 3.0f + cx) + 0.08f * vk_sin(a * 7.0f + cy * 2.0f));
                if (d < d1) { d1 = d; id = vk_h2(cx, cy, seed ^ 2u); ang = a; }
            }
            float band = d1 * 9.0f + 0.3f * vk_noise2(ang * 2.0f, d1 * 4.0f, seed) + t * 0.0015f;
            int ib = (int)floorf(band);
            float f = vk_fract(band);
            float lit = vk_h2(ib, (int)(id * 100.0f), seed ^ 7u);
            float on = vk_sstep(0.25f, 0.45f, lit + 0.1f * vk_sin(t * 0.002f + lit * 20.0f));
            float m = on * vk_sstep(0.0f, 0.15f, f) * vk_sstep(1.0f, 0.85f, f) * (0.55f + 0.45f * lit);
            float ci = base + lit * 1500.0f + id * 1400.0f + t * 0.4f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
