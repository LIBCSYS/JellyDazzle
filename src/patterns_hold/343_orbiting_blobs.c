/* pattern_343 — ORBITING BLOBS (ground): six soft radial bumps on their own
 * slow orbits, summed; hue from which blob dominates, light from the sum.
 * The classic "wobbling ellipses" plasma with all the speed taken out. */
#include "_gk336.h"

void pattern_343(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0025f;
    float hue0 = gk_sf(seed, 61) + t * 0.02f;
    float bx[6], by[6], bh[6];
    for (int i = 0; i < 6; i++) {
        float ph = gk_sf(seed, 70 + i) * 6.28f, sp = 0.4f + 0.5f * gk_sf(seed, 80 + i);
        bx[i] = GK_W * (0.5f + 0.38f * gk_sin(t * sp + ph));
        by[i] = GK_H * (0.5f + 0.38f * gk_cos(t * sp * 0.77f + ph * 1.3f));
        bh[i] = (float)i / 6.0f * 0.8f;
    }
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float sum = 0, hue = 0;
            for (int i = 0; i < 6; i++) {
                float dx = (float)x - bx[i], dy = (float)y - by[i];
                float f = 1.0f / (1.0f + (dx * dx + dy * dy) * 0.0004f);
                sum += f; hue += f * bh[i];
            }
            hue /= sum;
            float b = 0.50f + 0.50f * gk_clamp01(sum * 0.6f);
            gk_pix(y * GK_W + x, pal, hue0 + hue, b);
        }
    }
    gk_blit(fb, w, h);
}
