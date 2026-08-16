/* pattern_368 — POOL CAUSTICS (ground): the light net at the bottom of a
 * swimming pool — a Voronoi-like web made from the product of three soft
 * sine sheets, drifting slowly over a floor whose colour follows the ramp. */
#include "_gk336.h"

void pattern_368(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0035f;
    float hue0 = gk_sf(seed, 1) + t * 0.01f;
    float k = 0.05f + 0.02f * gk_sf(seed, 2);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float wx = gk_n3(fx * 0.01f, fy * 0.01f, t * 0.4f) * 30.0f;
            float wy = gk_n3(fx * 0.01f + 9.0f, fy * 0.01f, t * 0.4f + 3.0f) * 30.0f;
            float px = fx + wx, py = fy + wy;
            float a = gk_sin(px * k + t) , b = gk_sin((px * 0.5f + py * 0.866f) * k - t * 0.8f);
            float c = gk_sin((px * 0.5f - py * 0.866f) * k + t * 0.6f);
            float web = 1.0f - gk_absf(a * b * c);
            web = web * web * web * web;                     /* thin bright lines */
            float floor_ = gk_fbm3(fx * 0.006f, fy * 0.006f, t * 0.2f, 3);
            uint32_t base = gk_pal(pal, hue0 + floor_ * 0.15f + fy * 0.0004f);
            uint32_t light = gk_lift(gk_pal(pal, hue0 + 0.35f), 0.35f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(base, light, web * 0.8f), 0.6f + 0.15f * floor_ + 0.3f * web));
        }
    }
    gk_blit(fb, w, h);
}
