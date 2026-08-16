/* pattern_436 — WET WASHES (ground): broad overlapping washes of colour
 * laid wet — big soft-edged lobes (low-frequency noise thresholds), each
 * wash translucent over the ones below, edges pooling slightly darker. */
#define GK_W 256
#define GK_H 192
#include "_gk336.h"

void pattern_436(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 5) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x * 0.0075f, fy = (float)y * 0.0075f;
            float paper = gk_fbm2((float)x * 0.04f, (float)y * 0.04f, 2) * 0.05f;
            uint32_t col = gk_lift(gk_pal(pal, hue0 + paper), 0.25f);
            float lit = 0.92f + paper;
            for (int k = 0; k < 4; k++) {
                float n = gk_fbm3(fx * (1.0f + 0.3f * k) + (float)k * 7.0f, fy * (1.0f + 0.3f * k), t * 0.3f + (float)k, 3);
                float a = gk_sstep(-0.05f, 0.25f, n);
                float edge = expf(-(n - 0.1f) * (n - 0.1f) * 150.0f) * 0.2f;
                uint32_t pig = gk_pal(pal, hue0 + 0.15f + (float)k * 0.2f + n * 0.05f);
                col = gk_mix(col, pig, a * 0.8f);
                lit -= edge * a;
            }
            gk_put(y * GK_W + x, gk_shade(col, lit));
        }
    }
    gk_blit(fb, w, h);
}
