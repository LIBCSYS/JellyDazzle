/* pattern_344 — SILK BANDS (ground): broad soft diagonal bands from a
 * single slow sine, modulated by a second at a slightly different angle so
 * they interfere into a moire of wide light and dark ribbons. */
#include "_gk336.h"

void pattern_344(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0026f;
    float hue0 = gk_sf(seed, 91) + t * 0.02f;
    float a1 = gk_sf(seed, 92) * 3.14f + t * 0.05f, a2 = a1 + 0.35f + 0.2f * gk_sin(t * 0.3f);
    float c1 = gk_cos(a1), s1 = gk_sin(a1), c2 = gk_cos(a2), s2 = gk_sin(a2);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u1 = fx * c1 + fy * s1, u2 = fx * c2 + fy * s2;
            float b1 = gk_sin(u1 * 0.05f + t), b2 = gk_sin(u2 * 0.043f - t * 0.7f);
            float b3 = gk_sin((u1 - u2) * 0.012f + t * 0.4f);
            float v = 0.70f + 0.18f * b1 * b2 + 0.12f * b3;
            gk_pix(y * GK_W + x, pal, hue0 + b1 * 0.05f + b3 * 0.08f, v);
        }
    }
    gk_blit(fb, w, h);
}
