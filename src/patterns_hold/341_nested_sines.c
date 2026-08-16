/* pattern_341 — NESTED SINES (ground): sin(sin(sin(x))) — the plasma noodle.
 * Nesting makes broad plateaus separated by soft creases; the creases wander
 * as the inner phases turn.  Colour follows the crease depth. */
#include "_gk336.h"

void pattern_341(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0028f;
    float hue0 = gk_sf(seed, 41) + t * 0.02f;
    float k = 0.012f + 0.006f * gk_sf(seed, 42);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float a = gk_sin(fx * k + t) * 2.2f + gk_sin(fy * k * 1.3f - t * 0.6f) * 2.0f;
            float b = gk_sin(a + fy * k * 0.7f + t * 0.4f) * 2.0f + gk_sin(fx * k * 0.5f - a * 0.5f);
            float c = gk_sin(b * 1.5f + a * 0.5f + t * 0.3f);
            float v = 0.72f + 0.22f * c + 0.06f * gk_sin(b);
            gk_pix(y * GK_W + x, pal, hue0 + b * 0.05f + c * 0.06f, v);
        }
    }
    gk_blit(fb, w, h);
}
