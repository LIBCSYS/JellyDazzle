/* pattern_462 — SOAP FILM (ground): the swirling colour bands of a soap
 * film — a slow domain-warped thickness field mapped to the ramp with
 * several cycles, so hues wrap into iridescent swirls; sheen on top. */
#include "_gk336.h"

void pattern_462(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.001f;
    float hue0 = gk_sf(seed, 25) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float qx = gk_fbm3(fx * 1.5f, fy * 1.5f, t * 0.3f, 3), qy = gk_fbm3(fx * 1.5f + 7.0f, fy * 1.5f + 3.0f, t * 0.3f, 3);
            float thick = gk_fbm3(fx * 1.2f + qx * 1.5f, fy * 1.2f + qy * 1.5f + fy * 1.5f, t * 0.2f, 3);
            float sheen = gk_sstep(0.2f, 0.5f, gk_n3(fx * 2.0f - t * 0.5f, fy * 2.0f, t * 0.3f)) * 0.3f;
            uint32_t c = gk_pal(pal, hue0 + thick * 0.9f + fy * 0.2f);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, sheen), 0.65f + 0.25f * (thick * 0.5f + 0.5f) + 0.1f * qx));
        }
    }
    gk_blit(fb, w, h);
}
