/* pattern_376 — SAND RIPPLES (ground): wind ripples in sand — parallel
 * asymmetric ridges (steep lee side) bent by low-frequency noise, lit
 * from a low grazing sun that circles very slowly. */
#include "_gk336.h"

void pattern_376(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 41) + t * 0.01f;
    float ang = gk_sf(seed, 42) * 3.14f + t * 0.1f, ca = gk_cos(ang), sa = gk_sin(ang);
    float sun = t * 0.5f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float u = fx * ca + fy * sa;
            float bend = gk_n2(fx * 0.006f, fy * 0.006f) * 40.0f + gk_n3(fx * 0.02f, fy * 0.02f, t * 0.3f) * 6.0f;
            float ph = (u + bend) * 0.11f;
            float saw = gk_fract(ph / 6.2832f);
            float ridge = saw < 0.7f ? saw / 0.7f : (1.0f - saw) / 0.3f;   /* asymmetric */
            ridge = ridge * ridge * (3.0f - 2.0f * ridge);
            float slope = saw < 0.7f ? 1.0f : -2.3f;
            float lit = 0.5f + 0.35f * slope * gk_cos(sun - ang) * 0.4f + 0.25f * ridge;
            float grain = gk_fbm2(fx * 0.04f, fy * 0.04f, 2) * 0.05f;
            uint32_t c = gk_pal(pal, hue0 + ridge * 0.1f + grain + gk_n2(fx * 0.004f, fy * 0.004f) * 0.08f);
            gk_put(y * GK_W + x, gk_shade(c, 0.55f + 0.4f * gk_clamp01(lit) + grain));
        }
    }
    gk_blit(fb, w, h);
}
