/* pattern_468 — HUE CLOUDS (ground): two independent slow noise fields —
 * one drives the ramp position, one the light — so colour and brightness
 * clouds drift through each other and the frame never settles into a
 * single hue. Big scale, very slow: a resting background. */
#include "_gk336.h"

void pattern_468(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.001f;
    float hue0 = gk_sf(seed, 79) + t * 0.008f;
    float sc = 0.004f + 0.002f * gk_sf(seed, 80);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float hue = gk_fbm3(fx * sc, fy * sc, t * 0.3f, 3);
            float lum = gk_fbm3(fx * sc * 1.4f + 11.0f, fy * sc * 1.4f + 5.0f, t * 0.25f + 3.0f, 3);
            float detail = gk_n3(fx * 0.02f, fy * 0.02f, t * 0.4f) * 0.05f;
            uint32_t c = gk_pal(pal, hue0 + hue * 0.6f + detail);
            gk_put(y * GK_W + x, gk_shade(c, 0.7f + 0.28f * lum + detail));
        }
    }
    gk_blit(fb, w, h);
}
