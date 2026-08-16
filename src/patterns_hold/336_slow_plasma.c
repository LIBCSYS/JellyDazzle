/* pattern_336 — SLOW PLASMA (ground): four broad sine fields drifting on
 * incommensurate clocks, colour from their sum, brightness from a slower
 * second sum so the hue field and the light field never line up. */
#include "_gk336.h"

void pattern_336(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0045f;
    float k1 = 0.018f + 0.008f * gk_sf(seed, 1), k2 = 0.014f + 0.007f * gk_sf(seed, 2);
    float k3 = 0.011f + 0.006f * gk_sf(seed, 3);
    float hue0 = gk_sf(seed, 4) + t * 0.02f;
    float cx = GK_W * 0.5f + 60.0f * gk_sin(t * 0.7f), cy = GK_H * 0.5f + 40.0f * gk_cos(t * 0.53f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float dx = fx - cx, dy = fy - cy;
            float c = gk_sin(fx * k1 + t) + gk_sin(fy * k2 - t * 0.8f)
                    + gk_sin((fx + fy) * k3 * 0.7f + t * 0.6f)
                    + gk_sin(sqrtf(dx * dx + dy * dy) * 0.03f - t * 1.1f);
            float v = 0.72f + 0.22f * gk_sin(c * 1.3f + t * 0.4f) + 0.08f * gk_sin(fy * 0.02f + t * 0.3f);
            gk_pix(y * GK_W + x, pal, hue0 + c * 0.075f, v);
        }
    }
    gk_blit(fb, w, h);
}
