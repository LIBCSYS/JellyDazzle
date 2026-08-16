/* pattern_393 — GROUTED TILES (ground): a square tile grid with soft grout
 * lines; tiles take colours from the ramp in slow drifting waves (so
 * neighbours harmonise), each tile bevelled with a light that circles. */
#include "_gk336.h"

void pattern_393(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 5) + t * 0.008f;
    float cell = 24.0f + 12.0f * gk_sf(seed, 6);
    float lx = gk_cos(t * 0.6f), ly = gk_sin(t * 0.6f);
    float ang = 0.05f * gk_sin(t * 0.2f), ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = (fx * ca + fy * sa) / cell, v = (-fx * sa + fy * ca) / cell;
            int ui = (int)floorf(u), vi = (int)floorf(v);
            float pu = u - (float)ui - 0.5f, pv = v - (float)vi - 0.5f;
            float edge = gk_sstep(0.5f, 0.42f, gk_absf(pu)) * gk_sstep(0.5f, 0.42f, gk_absf(pv));
            float bevel = (pu * lx + pv * ly) * 0.6f;
            float wave = gk_n3((float)ui * 0.15f, (float)vi * 0.15f, t * 0.4f);
            uint32_t tile = gk_pal(pal, hue0 + wave * 0.5f + gk_hf(gk_hash2(ui, vi, seed)) * 0.03f);
            uint32_t grout = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.6f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(grout, tile, edge), 0.7f + 0.2f * bevel * edge + 0.1f * wave));
        }
    }
    gk_blit(fb, w, h);
}
