/* pattern_370 — DEEP WATER (ground): looking down through deep water —
 * layered slow ripples at three depths, each a soft interference of two
 * sine sheets, the deeper layers dimmer and bluer along the ramp. */
#include "_gk336.h"

void pattern_370(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.003f;
    float hue0 = gk_sf(seed, 9) + t * 0.01f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float sum = 0, hue = 0;
            for (int k = 0; k < 3; k++) {
                float sc = 0.02f + 0.012f * (float)k, ph = (float)k * 2.1f;
                float d = gk_n3(fx * 0.008f + ph, fy * 0.008f, t * 0.3f + ph) * 20.0f;
                float r = gk_sin((fx + d) * sc + t * (0.6f + 0.2f * k) + ph) * gk_sin((fy - d) * sc * 1.3f - t * 0.5f + ph);
                float wgt = 1.0f - 0.25f * (float)k;
                sum += (r * 0.5f + 0.5f) * wgt; hue += (r * 0.5f + 0.5f) * 0.06f * (float)(k + 1);
            }
            sum /= 2.25f;
            uint32_t c = gk_pal(pal, hue0 + hue + fy * 0.0005f);
            gk_put(y * GK_W + x, gk_shade(c, 0.55f + 0.42f * sum));
        }
    }
    gk_blit(fb, w, h);
}
