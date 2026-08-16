/* pattern_451 — TOPO CONTOURS (ground): a topographic map — soft contour
 * lines on a height field, filled hypsometric tints between them, the
 * terrain very slowly morphing so contours creep and merge. */
#include "_gk336.h"

void pattern_451(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0008f;
    float hue0 = gk_sf(seed, 21) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x * 0.006f, fy = (float)y * 0.006f;
            float hgt = gk_fbm3(fx, fy, t * 0.3f, 4) * 0.5f + 0.5f;    /* 0..1 */
            float lv = hgt * 12.0f;
            float fr = gk_fract(lv);
            float line = 1.0f - gk_sstep(0.0f, 0.22f, fr) * gk_sstep(1.0f, 0.78f, fr);
            float major = (((int)floorf(lv)) % 4 == 0) ? 1.0f : 0.5f;
            uint32_t tint = gk_pal(pal, hue0 + hgt * 0.5f);
            uint32_t ink = gk_shade(gk_pal(pal, hue0 + 0.55f), 0.5f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(tint, ink, line * major * 0.9f), 0.75f + 0.2f * hgt + 0.05f * fr));
        }
    }
    gk_blit(fb, w, h);
}
