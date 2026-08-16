/* pattern_467 — CHROME RIPPLES (ground): a rippled chrome sheet
 * reflecting a striped sky — the reflection is a lookup of broad colour
 * stripes by the ripple normal, so bands bend and shimmer without haste. */
#include "_gk336.h"

void pattern_467(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0025f;
    float hue0 = gk_sf(seed, 75) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            #define HC(X, Y) (gk_sin((X) * 0.025f + (Y) * 0.01f + t) * 0.5f + gk_sin((X) * 0.01f - (Y) * 0.03f - t * 0.7f) * 0.5f + gk_n3((X) * 0.012f, (Y) * 0.012f, t * 0.3f) * 0.6f)
            float h0 = HC(fx, fy);
            float dxh = HC(fx + 1.5f, fy) - h0, dyh = HC(fx, fy + 1.5f) - h0;
            #undef HC
            float ny = dyh * 6.0f, nx = dxh * 6.0f;
            /* environment: horizontal stripes indexed by reflected "elevation" */
            float elev = fy / GK_H + ny * 0.5f;
            float stripe = gk_sin(elev * 9.0f + t * 0.5f) * 0.5f + 0.5f;
            stripe = gk_sstep(0.3f, 0.7f, stripe);
            uint32_t sky = gk_pal(pal, hue0 + elev * 0.3f + nx * 0.05f);
            uint32_t sky2 = gk_pal(pal, hue0 + 0.4f + elev * 0.2f);
            uint32_t c = gk_mix(sky, sky2, stripe);
            float spec = gk_clamp01(1.0f - gk_absf(nx + ny - 0.4f) * 3.0f) * 0.4f;
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, spec), 0.6f + 0.25f * stripe + 0.15f * (1.0f - elev)));
        }
    }
    gk_blit(fb, w, h);
}
