/* pattern_234 — WAVE TERRACES (field): rolling terrain seen from above as
 * terraced contour bands, each terrace lit and edged, black risers between,
 * the whole landscape heaving slowly.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_234(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float tz = t * 0.0015f;
    const float nlev = vk_seedr(seed, 1, 7.0f, 11.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * 2.0f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 2.7f;
            float hgt = vk_fbm3(u, v, tz, 3, seed);
            float lv = hgt * nlev;
            float f = vk_fract(lv);
            int il = (int)floorf(lv);
            /* terrace: flat top lit, riser black */
            float m = vk_sstep(0.0f, 0.30f, f) * vk_sstep(1.0f, 0.65f, f) * (0.6f + 0.4f * hgt) * vk_sstep(0.40f, 0.55f, hgt);
            float ci = base + il * 380.0f + f * 400.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
