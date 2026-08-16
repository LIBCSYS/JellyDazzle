/* pattern_340 — POLAR PLASMA (ground): sines in radius and angle instead of
 * x and y — a slow petal-plasma that turns about a wandering centre. */
#include "_gk336.h"

void pattern_340(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0032f;
    float hue0 = gk_sf(seed, 31) + t * 0.02f;
    int petals = 3 + (int)(gk_sf(seed, 32) * 4.0f);
    float cx = GK_W * 0.5f + 50.0f * gk_sin(t * 0.43f), cy = GK_H * 0.5f + 35.0f * gk_cos(t * 0.31f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float c = gk_sin(r * 0.035f - t) + gk_sin(a * (float)petals + t * 0.5f) * (1.0f - expf(-r * 0.03f))
                    + gk_sin(r * 0.015f + a * 2.0f - t * 0.7f) * (1.0f - expf(-r * 0.03f)) + gk_sin((float)x * 0.02f + t * 0.3f);
            float b = 0.72f + 0.24f * gk_sin(c * 1.2f);
            gk_pix(y * GK_W + x, pal, hue0 + c * 0.08f, b);
        }
    }
    gk_blit(fb, w, h);
}
