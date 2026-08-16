/* pattern_378 — WIND STREAKS (ground): long soft streaks of blown sand
 * running diagonally — anisotropic noise (stretched 12:1 along the wind)
 * with a slow drift, over a mottled base of a second hue. */
#include "_gk336.h"

void pattern_378(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.003f;
    float hue0 = gk_sf(seed, 49) + t * 0.01f;
    float ang = 0.2f + 0.6f * gk_sf(seed, 50), ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            float s = gk_n3(u * 0.004f - t * 0.6f, v * 0.05f, t * 0.1f) * 0.6f
                    + gk_n3(u * 0.01f - t * 1.0f + 7.0f, v * 0.12f, t * 0.15f) * 0.4f;
            float base = gk_fbm3(fx * 0.007f, fy * 0.007f, t * 0.2f, 3);
            uint32_t cb = gk_pal(pal, hue0 + base * 0.15f);
            uint32_t cs = gk_pal(pal, hue0 + 0.3f + s * 0.1f);
            uint32_t c = gk_mix(cb, cs, gk_sstep(-0.2f, 0.6f, s) * 0.8f);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.25f * s + 0.12f * base + 0.1f));
        }
    }
    gk_blit(fb, w, h);
}
