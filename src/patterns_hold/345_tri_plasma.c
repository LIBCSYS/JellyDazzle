/* pattern_345 — TRI PLASMA (ground): three sines along three axes 120
 * degrees apart — a hexagonal plasma whose bright cells sit in a honeycomb
 * that slowly drifts and rotates. */
#include "_gk336.h"

void pattern_345(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0030f;
    float hue0 = gk_sf(seed, 101) + t * 0.02f;
    float rot = t * 0.08f + gk_sf(seed, 102) * 6.0f;
    float k = 0.035f + 0.02f * gk_sf(seed, 103);
    float ax[3], ay[3];
    for (int i = 0; i < 3; i++) { ax[i] = gk_cos(rot + (float)i * 2.0944f); ay[i] = gk_sin(rot + (float)i * 2.0944f); }
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float c = 0;
            for (int i = 0; i < 3; i++) c += gk_sin((fx * ax[i] + fy * ay[i]) * k + t * (0.6f + 0.2f * (float)i));
            float lit = gk_sin(fx * 0.01f + t * 0.5f) * gk_sin(fy * 0.013f - t * 0.4f);
            float v = 0.72f + 0.20f * (c / 3.0f) + 0.10f * lit;
            gk_pix(y * GK_W + x, pal, hue0 + c * 0.06f + lit * 0.05f, v);
        }
    }
    gk_blit(fb, w, h);
}
