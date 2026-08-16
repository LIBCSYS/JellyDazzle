/* pattern_405 — VELVET NAP (ground): the streaks a hand leaves brushing
 * velvet — long soft strokes at a slow-turning angle, each stroke a band
 * that shifts from deep to bright as it crosses the nap direction. */
#include "_gk336.h"

void pattern_405(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 5) + t * 0.008f;
    float ang = gk_sf(seed, 6) * 3.14f + 0.15f * gk_sin(t * 0.3f), ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            float strokes = gk_n3(u * 0.003f, v * 0.03f, t * 0.2f) * 0.6f + gk_n3(u * 0.008f + 6.0f, v * 0.06f, t * 0.3f) * 0.4f;
            float bright = gk_sstep(-0.5f, 0.6f, strokes);
            float body = gk_n3((float)x * 0.005f, (float)y * 0.005f, t * 0.2f);
            uint32_t deep = gk_pal(pal, hue0 + body * 0.3f + strokes * 0.1f);
            uint32_t lit = gk_pal(pal, hue0 + 0.25f + strokes * 0.1f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(deep, lit, bright), 0.55f + 0.4f * bright + 0.05f * body));
        }
    }
    gk_blit(fb, w, h);
}
