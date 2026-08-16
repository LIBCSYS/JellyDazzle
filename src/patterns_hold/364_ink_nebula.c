/* pattern_364 — INK NEBULA (ground): domain-warped fBm — noise fed back
 * into its own coordinates twice — which gives the curling, folded look of
 * a real emission nebula. Three hue stops keyed to the warp magnitude. */
#define GK_W 256
#define GK_H 192
#include "_gk336.h"

void pattern_364(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 65) + t * 0.01f;
    float ox = gk_sf(seed, 66) * 30.0f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float qx = gk_fbm3(fx * 2.0f + ox, fy * 2.0f, t * 0.3f, 3);
            float qy = gk_fbm3(fx * 2.0f + ox + 5.2f, fy * 2.0f + 1.3f, t * 0.3f + 2.0f, 3);
            float rx = gk_fbm3(fx * 2.0f + ox + 4.0f * qx + 1.7f, fy * 2.0f + 4.0f * qy + 9.2f, t * 0.25f, 3);
            float ry = gk_fbm3(fx * 2.0f + ox + 4.0f * qx + 8.3f, fy * 2.0f + 4.0f * qy + 2.8f, t * 0.25f + 4.0f, 3);
            float f = gk_fbm3(fx * 2.0f + ox + 4.0f * rx, fy * 2.0f + 4.0f * ry, t * 0.2f, 3);
            float qm = sqrtf(qx * qx + qy * qy), rm = sqrtf(rx * rx + ry * ry);
            uint32_t c = gk_pal(pal, hue0 + f * 0.25f);
            c = gk_mix(c, gk_pal(pal, hue0 + 0.3f + qm * 0.1f), gk_clamp01(qm * 1.5f));
            c = gk_mix(c, gk_pal(pal, hue0 + 0.55f + rm * 0.1f), gk_clamp01(rm * 1.2f) * 0.7f);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.35f * (f * 0.5f + 0.5f) + 0.1f * rm));
        }
    }
    gk_blit(fb, w, h);
}
