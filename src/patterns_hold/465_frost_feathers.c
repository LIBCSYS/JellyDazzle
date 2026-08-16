/* pattern_465 — FROST FEATHERS (ground): window frost — feathery ridged
 * noise stretched along a slowly turning growth direction, bright crystal
 * ridges over a cool tinted pane, the ridges creeping outward. */
#include "_gk336.h"

void pattern_465(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0008f;
    float hue0 = gk_sf(seed, 45) + t * 0.008f;
    float ang = gk_sf(seed, 46) * 3.14f + 0.1f * gk_sin(t * 0.3f), ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            float bend = gk_n3(u * 0.005f, v * 0.005f, t * 0.2f) * 40.0f;
            float feather = gk_ridge3((u + bend) * 0.012f, v * 0.03f, t * 0.3f, 4);
            float feather2 = gk_ridge3(u * 0.03f + 7.0f, (v - bend) * 0.012f, t * 0.25f, 3);
            float crys = gk_sstep(0.35f, 0.9f, feather) * 0.7f + gk_sstep(0.5f, 0.95f, feather2) * 0.4f;
            uint32_t pane = gk_pal(pal, hue0 + gk_n3(fx * 0.004f, fy * 0.004f, t * 0.2f) * 0.15f);
            uint32_t ice = gk_lift(gk_pal(pal, hue0 + 0.3f), 0.5f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(pane, ice, gk_clamp01(crys)), 0.6f + 0.35f * gk_clamp01(crys) + 0.05f * feather));
        }
    }
    gk_blit(fb, w, h);
}
