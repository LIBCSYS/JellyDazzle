/* pattern_439 — INK BLOTCHES (ground): pigment blotches with dark hard-
 * ish rims where the wash dried (edge pooling), interiors pale, laid over
 * each other; blotches breathe in size, drift, and rotate imperceptibly. */
#include "_gk336.h"

void pattern_439(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 17) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x * 0.008f, fy = (float)y * 0.008f;
            float paper = gk_fbm2((float)x * 0.04f, (float)y * 0.04f, 2) * 0.05f;
            uint32_t col = gk_lift(gk_pal(pal, hue0 + paper), 0.4f);
            float lit = 0.92f + paper;
            for (int k = 0; k < 3; k++) {
                float n = gk_fbm3(fx * (0.8f + 0.25f * k) + (float)k * 11.0f, fy * (0.8f + 0.25f * k) + 3.0f, t * 0.25f + (float)k * 2.0f, 3);
                float thr = 0.05f + 0.05f * gk_sin(t * 0.7f + (float)k);
                float a = gk_sstep(thr - 0.03f, thr + 0.03f, n);
                float rim = expf(-(n - thr) * (n - thr) * 900.0f);
                uint32_t pig = gk_pal(pal, hue0 + 0.2f + (float)k * 0.25f);
                col = gk_mix(col, gk_lift(pig, 0.35f), a * 0.7f);
                col = gk_mix(col, pig, rim * 0.8f);
                lit -= rim * 0.2f;
            }
            gk_put(y * GK_W + x, gk_shade(col, lit));
        }
    }
    gk_blit(fb, w, h);
}
