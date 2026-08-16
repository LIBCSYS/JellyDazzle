/* pattern_444 — INK WASH GRADIENT (ground): a sumi wash — graded tone
 * from top to bottom with brush-stroke banding, mottled where the wash
 * pooled, coloured from two ramp stops; the gradient tilts very slowly. */
#include "_gk336.h"

void pattern_444(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.001f;
    float hue0 = gk_sf(seed, 49) + t * 0.008f;
    float ang = 1.5708f + 0.3f * gk_sin(t * 0.3f), ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float g = (fx * ca + fy * sa) / GK_H + 0.5f;         /* 0..1 along gradient */
            float u = -fx * sa + fy * ca;
            float strokes = gk_n3(g * 12.0f, u * 0.006f, t * 0.4f) * 0.5f + gk_n3(g * 30.0f + 5.0f, u * 0.003f, t * 0.3f) * 0.25f;
            float pool = gk_fbm3((float)x * 0.01f, (float)y * 0.01f, t * 0.3f, 3);
            float tone = gk_clamp01(g + strokes * 0.15f + pool * 0.1f);
            uint32_t light = gk_lift(gk_pal(pal, hue0), 0.4f);
            uint32_t deep = gk_pal(pal, hue0 + 0.35f + pool * 0.05f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(light, deep, tone), 0.75f + 0.15f * (1.0f - tone) + 0.08f * strokes));
        }
    }
    gk_blit(fb, w, h);
}
