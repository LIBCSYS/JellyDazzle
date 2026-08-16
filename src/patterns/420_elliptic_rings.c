/* pattern_420 — ELLIPTIC RINGS (ground): nested ellipses that slowly turn
 * and change eccentricity — soft banded rings whose colour cycles round the
 * ramp with each ring, lit brighter toward the middle. */
#include "_gk336.h"

void pattern_420(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 37) + t * 0.008f;
    float rot = t * 0.4f, cr = gk_cos(rot), sr = gk_sin(rot);
    float ecc = 1.6f + 0.6f * gk_sin(t * 0.3f);
    float cx = GK_W * 0.5f, cy = GK_H * 0.5f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float u = dx * cr - dy * sr, v = (dx * sr + dy * cr) * ecc;
            float r = sqrtf(u * u + v * v);
            float ph = r * 0.05f - t;
            float ring = gk_cos(ph) * 0.5f + 0.5f;
            float idx = ph * (1.0f / 6.2832f);
            uint32_t c = gk_pal(pal, hue0 + idx * 0.06f + ring * 0.12f);
            gk_put(y * GK_W + x, gk_shade(c, 0.55f + 0.3f * ring + 0.15f * expf(-r * 0.006f)));
        }
    }
    gk_blit(fb, w, h);
}
