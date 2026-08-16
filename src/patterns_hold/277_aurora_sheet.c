/* pattern_277 — AURORA SHEET (field): a broad aurora band lying across the
 * middle of the sky, seen from below — vertical striations rippling along
 * it, its lower edge bright, fading upward; black sky above and below.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_277(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.003f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw;
            /* band centreline wanders as a slow S */
            float cy = 0.5f + 0.15f * vk_sin(u * 4.0f + t * 0.0015f) + 0.06f * vk_sin(u * 9.0f - t * 0.0022f);
            float thick = 0.34f + 0.10f * vk_noise2(u * 3.0f, tz * 0.5f, seed);
            float d = v - cy;
            /* bright lower edge, soft fade upward (d<0 = above) */
            float prof = d > 0.0f ? vk_sstep(thick * 0.6f, 0.0f, d) : vk_sstep(-thick * 1.2f, -thick * 0.1f, d) * 0.9f;
            float stri = vk_noise2(u * 40.0f + tz * 0.7f, tz * 0.4f, seed ^ 3u) * 0.6f + vk_noise2(u * 12.0f - tz * 0.4f, tz, seed ^ 5u) * 0.4f;
            float m = prof * (0.5f + 0.5f * vk_sstep(0.15f, 0.7f, stri));
            float ci = base + (d + thick) * 6000.0f + stri * 700.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, stri, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
