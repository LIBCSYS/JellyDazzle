/* pattern_428 — SOFT TARTAN (ground): plaid — bands of varying width in
 * two directions, crossing translucently, drawn with sine profiles so the
 * edges are soft, and the sett rippling gently as if on cloth. */
#include "_gk336.h"

void pattern_428(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 17) + t * 0.008f;
    float f1 = 0.03f + 0.01f * gk_sf(seed, 18), f2 = f1 * 2.7f, f3 = f1 * 0.45f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float u = fx + 6.0f * gk_sin(fy * 0.02f + t), v = fy + 6.0f * gk_sin(fx * 0.02f - t * 0.8f);
            float bu = gk_sstep(-0.3f, 0.3f, gk_sin(u * f1)) * 0.5f + gk_sstep(0.0f, 0.5f, gk_sin(u * f2 + 1.0f)) * 0.25f + gk_sstep(-0.2f, 0.4f, gk_sin(u * f3)) * 0.35f;
            float bv = gk_sstep(-0.3f, 0.3f, gk_sin(v * f1)) * 0.5f + gk_sstep(0.0f, 0.5f, gk_sin(v * f2 + 1.0f)) * 0.25f + gk_sstep(-0.2f, 0.4f, gk_sin(v * f3)) * 0.35f;
            uint32_t cu = gk_pal(pal, hue0 + bu * 0.3f), cv = gk_pal(pal, hue0 + 0.4f + bv * 0.3f);
            uint32_t c = gk_mix(cu, cv, 0.5f);
            float weave = gk_n2(fx * 0.05f, fy * 0.05f) * 0.04f;
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.2f * bu + 0.2f * bv + weave));
        }
    }
    gk_blit(fb, w, h);
}
