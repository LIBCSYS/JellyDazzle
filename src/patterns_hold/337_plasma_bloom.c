/* pattern_337 — PLASMA BLOOM (ground): plasma whose colour is bent through
 * a soft radial lens, so the middle blooms out in one hue family and the
 * corners settle into another. Two lens centres orbit slowly. */
#include "_gk336.h"

void pattern_337(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0038f;
    float hue0 = gk_sf(seed, 7) + t * 0.015f;
    float ax = GK_W * (0.5f + 0.25f * gk_sin(t * 0.61f)), ay = GK_H * (0.5f + 0.25f * gk_cos(t * 0.47f));
    float bx = GK_W * (0.5f + 0.3f * gk_cos(t * 0.39f + 2.0f)), by = GK_H * (0.5f + 0.3f * gk_sin(t * 0.52f + 1.0f));
    float sp = 0.5f + 0.4f * gk_sf(seed, 8);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float da = sqrtf((fx - ax) * (fx - ax) + (fy - ay) * (fy - ay));
            float db = sqrtf((fx - bx) * (fx - bx) + (fy - by) * (fy - by));
            float c = gk_sin(da * 0.028f - t) + gk_sin(db * 0.021f + t * 0.7f)
                    + gk_sin((fx * 0.9f - fy * 0.5f) * 0.017f + t * 0.5f);
            float bloom = expf(-da * da * 0.00006f) * 0.5f;
            float v = 0.76f + 0.20f * gk_sin(c * 1.1f) + bloom * 0.3f;
            gk_pix(y * GK_W + x, pal, hue0 + c * 0.12f * sp + bloom * 0.25f, v);
        }
    }
    gk_blit(fb, w, h);
}
