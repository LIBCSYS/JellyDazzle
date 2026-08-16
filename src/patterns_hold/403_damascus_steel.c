/* pattern_403 — DAMASCUS STEEL (ground): the layered pattern-welded look —
 * many thin contour bands of a warped height field, alternately light and
 * dark, flowing like wood grain in steel; the bands drift very slowly. */
#include "_gk336.h"

void pattern_403(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.001f;
    float hue0 = gk_sf(seed, 45) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float wv = gk_n3(fx * 0.006f, fy * 0.006f, t * 0.3f) * 3.0f + gk_n3(fx * 0.015f + 5.0f, fy * 0.015f, t * 0.4f) * 1.0f;
            float layers = gk_sin(fy * 0.09f + fx * 0.02f + wv * 4.0f);
            float band = layers * 0.5f + 0.5f;
            band = gk_sstep(0.3f, 0.7f, band);
            float sheen = gk_sin(fx * 0.01f + fy * 0.006f + t * 2.0f) * 0.5f + 0.5f;
            uint32_t light = gk_pal(pal, hue0 + wv * 0.03f + 0.02f);
            uint32_t dark = gk_pal(pal, hue0 + 0.3f + wv * 0.03f);
            uint32_t c = gk_mix(dark, light, band);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, sheen * 0.15f * band), 0.6f + 0.25f * band + 0.15f * sheen));
        }
    }
    gk_blit(fb, w, h);
}
