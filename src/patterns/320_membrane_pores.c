/* pattern_320 — MEMBRANE PORES (field): a soft luminous sheet perforated by
 * round pores that dilate and close on slow clocks, the sheet itself
 * rippling with light; the pores are black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_320(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = vk_seedr(seed, 1, 5.0f, 7.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float hole = 0.0f;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float h1 = vk_h2(cx, cy, seed), h2 = vk_h2(cx, cy, seed ^ 3u);
                float sx = cx + 0.25f + 0.5f * h1, sy = cy + 0.25f + 0.5f * h2;
                float dx = u - sx, dy = v - sy;
                float d = sqrtf(dx * dx + dy * dy);
                float rad = 0.32f * (0.5f + 0.5f * vk_sin(t * 0.003f + h1 * VK_TAU)) + 0.06f;
                float hh = vk_sstep(rad + 0.06f, rad, d);
                if (hh > hole) hole = hh;
            }
            float sheet = 1.0f - hole;
            float ripple = 0.5f + 0.5f * vk_sin(u * 1.5f + v * 1.2f + t * 0.003f) ;
            float ripple2 = 0.5f + 0.5f * vk_sin(u * 0.7f - v * 1.5f - t * 0.002f);
            /* dim panels so the sheet is not one flat wall */
            float m = sheet * (0.35f + 0.65f * vk_sstep(0.2f, 0.8f, ripple * 0.5f + ripple2 * 0.5f)) * (0.85f + 0.15f * ripple);
            float ci = base + ripple * 1500.0f + ripple2 * 1200.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, hole, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
