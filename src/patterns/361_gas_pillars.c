/* pattern_361 — GAS PILLARS (ground): tall columns of dense gas rising
 * from the bottom edge, their tops eroded by noise, back-lit by a bright
 * field above — a soft take on the pillars of creation. */
#include "_gk336.h"

void pattern_361(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 53) + t * 0.01f;
    float ox = gk_sf(seed, 54) * 40.0f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float col = gk_n2(fx * 2.2f + ox, 3.0f);                /* pillar tops per column */
            float top = 0.35f + 0.3f * col + 0.12f * gk_n3(fx * 6.0f + ox, fy * 3.0f, t * 0.4f);
            float inside = gk_sstep(top - 0.12f, top + 0.12f, fy);
            float tex = gk_fbm3(fx * 3.0f + ox, fy * 3.0f - t * 0.1f, t * 0.3f, 4);
            float back = gk_fbm3(fx * 1.5f + 9.0f, fy * 1.5f, t * 0.25f, 3);
            uint32_t sky = gk_pal(pal, hue0 + back * 0.15f + fy * 0.05f);
            uint32_t gas = gk_pal(pal, hue0 + 0.35f + tex * 0.12f);
            uint32_t c = gk_mix(sky, gas, inside);
            float rim = expf(-(fy - top) * (fy - top) * 120.0f) * 0.4f;
            gk_put(y * GK_W + x, gk_shade(c, 0.60f + 0.25f * back * (1.0f - inside) + 0.15f * tex + rim + 0.1f * (1.0f - inside)));
        }
    }
    gk_blit(fb, w, h);
}
