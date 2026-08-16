/* pattern_453 — BATHYMETRY (ground): a depth chart — deep basins to
 * shallow shelves in graded tints, soft depth contours, and a slow swell
 * moving over the whole field like light through water. */
#include "_gk336.h"

void pattern_453(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.001f;
    float hue0 = gk_sf(seed, 29) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x * 0.005f, fy = (float)y * 0.005f;
            float depth = gk_fbm3(fx, fy, t * 0.25f, 4) * 0.5f + 0.5f;
            float lv = depth * 8.0f, fr = gk_fract(lv);
            float line = 1.0f - gk_sstep(0.0f, 0.12f, fr) * gk_sstep(1.0f, 0.88f, fr);
            float swell = gk_sin((float)x * 0.02f + (float)y * 0.01f + t * 3.0f) * gk_sin((float)y * 0.025f - t * 2.0f) * 0.5f + 0.5f;
            uint32_t deep = gk_pal(pal, hue0 + depth * 0.5f);
            uint32_t ink = gk_shade(deep, 0.6f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(deep, ink, line * 0.7f), 0.55f + 0.25f * depth + 0.15f * swell));
        }
    }
    gk_blit(fb, w, h);
}
