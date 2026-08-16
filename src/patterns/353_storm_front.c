/* pattern_353 — STORM FRONT (ground): heavy rolling cloud with bright rims —
 * ridged noise gives the billows, a slow horizontal advection rolls them,
 * and a warm under-light leaks up from the bottom edge. */
#include "_gk336.h"

void pattern_353(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 21) + t * 0.01f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float r = gk_ridge3(fx * 2.0f + t * 0.3f, fy * 2.5f, t * 0.2f, 4);
            float body = gk_fbm3(fx * 1.2f - t * 0.2f, fy * 1.5f, t * 0.15f + 9.0f, 3);
            float under = (1.0f - fy) * (1.0f - fy);
            uint32_t cloud = gk_pal(pal, hue0 + body * 0.15f);
            uint32_t rim = gk_pal(pal, hue0 + 0.3f + r * 0.1f);
            uint32_t c = gk_mix(cloud, rim, gk_sstep(0.3f, 0.8f, r) * 0.7f);
            float lit = 0.55f + 0.30f * r + 0.15f * (1.0f - under) * body + 0.15f * (1.0f - fy) * 0.5f;
            gk_put(y * GK_W + x, gk_shade(c, lit));
        }
    }
    gk_blit(fb, w, h);
}
