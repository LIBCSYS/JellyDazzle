/* pattern_224 — FISH SCALES (field): overlapping rows of scales shimmer as
 * the fish flexes; every scale shades from bright rim to dark root, and a
 * slow flex sends waves of black shadow through the field.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_224(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 9.0f, 14.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 0.75f * n;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * n;
            /* flex: warp coordinates by a slow travelling wave */
            float uu = u + 0.6f * vk_sin(v * 0.6f + t * 0.003f), vv = v + 0.4f * vk_sin(u * 0.5f - t * 0.002f);
            /* rows of scales, alternate rows offset by half; the scale that
             * covers a point is the nearest centre in the row BELOW its top */
            float best = 9.0f; float rim = 0.0f; float id = 0.0f;
            for (int j = 0; j <= 1; j++) {
                float ry = floorf(vv) - j;                  /* row index */
                float ox = ((int)ry & 1) ? 0.5f : 0.0f;
                float cxr = floorf(uu - ox + 0.5f) + ox;
                float dx = uu - cxr, dy = vv - ry;
                float d = sqrtf(dx * dx + dy * dy * 0.85f);
                if (d < 0.75f && dy >= 0.0f) {
                    /* the upper (larger j... ) scale overlaps: prefer smaller dy (row on top = the lower row) */
                    if (j == 0 || best > 8.0f) { best = d; rim = 1.0f - d / 0.75f; id = vk_h2((int)cxr, (int)ry, seed); if (j == 0) break; }
                }
            }
            float m = 0.0f;
            if (best < 8.0f) {
                float edge = vk_sstep(0.0f, 0.10f, rim);
                float grad = 0.35f + 0.65f * (1.0f - rim);       /* bright rim, dark root */
                float flexlight = 0.5f + 0.5f * vk_sin(uu * 0.8f + vv * 0.6f - t * 0.005f);
                m = edge * grad * vk_sstep(0.05f, 0.5f, flexlight);
            }
            float ci = base + id * 900.0f + rim * 1500.0f + v * 100.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, rim, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
