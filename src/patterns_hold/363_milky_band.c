/* pattern_363 — MILKY BAND (ground): a diagonal band of dense luminous
 * cloud (the galactic plane) crossing a softly glowing sky, with darker
 * dust rifts threading the band and everything sliding at a crawl. */
#include "_gk336.h"

void pattern_363(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0014f;
    float hue0 = gk_sf(seed, 61) + t * 0.01f;
    float ang = 0.4f + 0.5f * gk_sf(seed, 62), ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H - 0.66f, fy = (float)y / GK_H - 0.5f;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            float n = gk_fbm3(u * 3.0f + t * 0.2f, v * 4.0f, t * 0.3f, 4);
            float rift = gk_ridge3(u * 5.0f - t * 0.15f, v * 8.0f, t * 0.2f + 7.0f, 3);
            float band = expf(-(v + 0.06f * n) * (v + 0.06f * n) * 18.0f);
            uint32_t sky = gk_pal(pal, hue0 + v * 0.2f + n * 0.05f);
            uint32_t milk = gk_lift(gk_pal(pal, hue0 + 0.35f + n * 0.12f), 0.25f);
            uint32_t c = gk_mix(sky, milk, band * (0.6f + 0.4f * n));
            float dust = 1.0f - 0.35f * band * gk_sstep(0.5f, 0.9f, rift);
            gk_put(y * GK_W + x, gk_shade(c, (0.55f + 0.4f * band + 0.1f * n) * dust));
        }
    }
    gk_blit(fb, w, h);
}
