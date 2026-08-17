/* pattern_450 — ONYX LAYERS (ground): banded onyx — parallel wavy strata
 * of alternating translucency, some bands thin and bright, some broad
 * and deep, the strata bending and slowly sliding sideways. */
#include "_gk336.h"

void pattern_450(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.001f;
    float hue0 = gk_sf(seed, 17) + t * 0.008f;
    float ang = gk_sf(seed, 18) * 0.8f - 0.4f, ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            float bend = gk_n3(u * 0.005f + t * 0.2f, v * 0.002f, t * 0.2f) * 30.0f + gk_n3(u * 0.015f, v * 0.005f, t * 0.3f) * 8.0f;
            float s = (v + bend) * 0.06f;
            float thin = powf(gk_sin(s * 3.0f) * 0.5f + 0.5f, 6.0f);
            float broad = gk_sin(s) * 0.5f + 0.5f;
            float broad2 = gk_sin(s * 0.37f + 1.0f) * 0.5f + 0.5f;
            uint32_t deep = gk_pal(pal, hue0 + broad2 * 0.2f);
            uint32_t mid = gk_pal(pal, hue0 + 0.3f + broad2 * 0.1f);
            uint32_t bright = gk_lift(gk_pal(pal, hue0 + 0.1f), 0.5f);
            uint32_t c = gk_mix(deep, mid, broad);
            c = gk_mix(c, bright, thin * 0.8f);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.25f * broad + 0.15f * thin));
        }
    }
    gk_blit(fb, w, h);
}
