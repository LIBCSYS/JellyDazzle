/* pattern_386 — QUILTED PADDING (ground): diamond-quilted upholstery —
 * a lattice of puffy lozenges, each a smooth dome with a dimple at the
 * corners, lit from a light that circles slowly so the puffs roll. */
#include "_gk336.h"

void pattern_386(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 21) + t * 0.01f;
    float ang = 0.785f + 0.1f * gk_sin(t * 0.3f), ca = gk_cos(ang), sa = gk_sin(ang);
    float cell = 46.0f + 16.0f * gk_sf(seed, 22);
    float lx = gk_cos(t * 0.6f), ly = gk_sin(t * 0.6f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = (fx * ca + fy * sa) / cell, v = (-fx * sa + fy * ca) / cell;
            float pu = gk_fract(u) - 0.5f, pv = gk_fract(v) - 0.5f;
            /* dome height: cos-shaped in both axes */
            float hu = gk_cos(pu * 3.14159f), hv = gk_cos(pv * 3.14159f);
            float dome = hu * hv;
            float gu = -gk_sin(pu * 3.14159f) * hv, gv = -hu * gk_sin(pv * 3.14159f);
            float nx = gu * ca - gv * sa, ny = gu * sa + gv * ca;
            float diff = gk_clamp01(0.5f + (nx * lx + ny * ly) * 0.55f);
            float seam = gk_sstep(0.0f, 0.15f, dome);
            float tint = gk_n3((float)x * 0.006f, (float)y * 0.006f, t * 0.2f);
            uint32_t c = gk_pal(pal, hue0 + tint * 0.15f + floorf(u) * 0.013f);
            gk_put(y * GK_W + x, gk_shade(c, (0.45f + 0.45f * diff + 0.1f * dome) * (0.7f + 0.3f * seam)));
        }
    }
    gk_blit(fb, w, h);
}
