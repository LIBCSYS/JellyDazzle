/* pattern_358 — NEBULA VEILS (ground): ridged fBm gas with a soft dust
 * lane through it, colour by density (thin gas one hue, dense knots
 * another), lit so the whole frame reads as glowing cloud, not black space. */
#include "_gk336.h"

void pattern_358(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0016f;
    float hue0 = gk_sf(seed, 41) + t * 0.01f;
    float ox = gk_sf(seed, 42) * 50.0f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float g = gk_ridge3(fx * 2.4f + ox, fy * 2.4f, t * 0.3f, 4);
            float d = gk_fbm3(fx * 1.3f + ox + 5.0f, fy * 1.3f, t * 0.2f, 3);
            float lane = 1.0f - 0.5f * expf(-(fy - 0.5f - 0.15f * gk_sin(fx * 3.0f + t)) * (fy - 0.5f - 0.15f * gk_sin(fx * 3.0f + t)) * 40.0f);
            uint32_t thin = gk_pal(pal, hue0 + d * 0.15f);
            uint32_t knot = gk_pal(pal, hue0 + 0.33f + g * 0.1f);
            uint32_t c = gk_mix(thin, knot, gk_sstep(0.25f, 0.85f, g));
            gk_put(y * GK_W + x, gk_shade(c, (0.5f + 0.5f * g) * lane * 0.9f + 0.1f));
        }
    }
    gk_blit(fb, w, h);
}
