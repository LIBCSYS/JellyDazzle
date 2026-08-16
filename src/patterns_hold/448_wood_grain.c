/* pattern_448 — WOOD GRAIN (ground): plank grain — growth rings from a
 * distant off-frame centre, distorted by noise into the classic cathedral
 * arches, with fine grain lines along the length; warm ramp stops. */
#include "_gk336.h"

void pattern_448(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0008f;
    float hue0 = gk_sf(seed, 9) + t * 0.008f;
    float ang = gk_sf(seed, 10) * 0.6f - 0.3f + 1.5708f, ca = gk_cos(ang), sa = gk_sin(ang);
    float cx = GK_W * 0.5f, cy = GK_H * (2.5f + gk_sf(seed, 11));   /* far centre */
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            float dx = u, dy = v + cy - GK_H * 0.5f;
            (void)cx;
            float r = sqrtf(dx * dx + dy * dy * 0.15f);
            r += gk_n3(u * 0.01f, v * 0.003f, t * 0.3f) * 25.0f + gk_n3(u * 0.03f + 7.0f, v * 0.01f, t * 0.4f) * 5.0f;
            float ring = gk_sin(r * 0.25f) * 0.5f + 0.5f;
            ring = powf(ring, 1.5f);
            float fine = gk_n2(u * 0.1f, v * 0.006f) * 0.05f;
            uint32_t light = gk_pal(pal, hue0 + fine);
            uint32_t dark = gk_pal(pal, hue0 + 0.22f + fine);
            gk_put(y * GK_W + x, gk_shade(gk_mix(light, dark, ring), 0.75f + 0.15f * (1.0f - ring) + fine));
        }
    }
    gk_blit(fb, w, h);
}
