/* pattern_382 — VELVET DRAPE (ground): velvet — deep colour that lights up
 * only where the pile is seen edge-on (a Fresnel-like rim on every fold),
 * so the folds glow at their turning edges and sink between. */
#include "_gk336.h"

void pattern_382(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 5) + t * 0.01f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            #define HV(X, Y) (gk_n3((X) * 0.006f, (Y) * 0.014f, t * 0.25f) + 0.5f * gk_n3((X) * 0.015f + 3.0f, (Y) * 0.03f, t * 0.3f))
            float h0 = HV(fx, fy);
            float dx = HV(fx + 2.0f, fy) - h0, dy = HV(fx, fy + 2.0f) - h0;
            #undef HV
            float slope = sqrtf(dx * dx + dy * dy) * 20.0f;
            float rim = gk_clamp01(slope);
            rim = rim * rim;
            uint32_t deep = gk_pal(pal, hue0 + h0 * 0.2f);
            uint32_t edge = gk_lift(gk_pal(pal, hue0 + 0.3f + h0 * 0.1f), 0.15f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(deep, edge, rim), 0.55f + 0.4f * rim + 0.05f * h0));
        }
    }
    gk_blit(fb, w, h);
}
