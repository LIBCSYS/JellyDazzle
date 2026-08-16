/* pattern_355 — HEAT SHIMMER (ground): a horizon gradient seen through
 * rising heat: fine slow vertical noise displaces the sample row, so bands
 * of colour waver like air over hot ground. Never sharp, never fast. */
#include "_gk336.h"

void pattern_355(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0035f;
    float hue0 = gk_sf(seed, 29) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float d = gk_n3(fx * 6.0f, fy * 4.0f - t * 1.2f, t * 0.4f) * 0.06f
                    + gk_n3(fx * 2.0f + 30.0f, fy * 1.5f - t * 0.6f, t * 0.2f) * 0.10f;
            float sy = fy + d * (0.4f + fy);
            float band = gk_sin(sy * 9.0f) * 0.5f + 0.5f;
            uint32_t c = gk_pal(pal, hue0 + sy * 0.3f + band * 0.04f);
            gk_put(y * GK_W + x, gk_shade(c, 0.62f + 0.25f * band + 0.10f * (1.0f - fy)));
        }
    }
    gk_blit(fb, w, h);
}
