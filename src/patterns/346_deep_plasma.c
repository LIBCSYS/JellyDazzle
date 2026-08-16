/* pattern_346 — DEEP PLASMA (ground): a plasma computed at two scales and
 * layered — big slow swells underneath, a finer ripple riding on top at a
 * fraction of the contrast.  Reads as depth rather than a flat field. */
#include "_gk336.h"

void pattern_346(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0030f;
    float hue0 = gk_sf(seed, 111) + t * 0.02f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float big = gk_sin(fx * 0.012f + t) + gk_sin(fy * 0.015f - t * 0.6f)
                      + gk_sin((fx - fy) * 0.009f + t * 0.4f);
            float fine = gk_sin(fx * 0.05f + big + t * 1.3f) + gk_sin(fy * 0.055f - big * 0.7f - t);
            float v = 0.70f + 0.20f * (big / 3.0f) * 1.5f + 0.08f * fine;
            gk_pix(y * GK_W + x, pal, hue0 + big * 0.12f + fine * 0.02f, v);
        }
    }
    gk_blit(fb, w, h);
}
