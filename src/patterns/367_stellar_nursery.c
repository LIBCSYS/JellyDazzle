/* pattern_367 — STELLAR NURSERY (ground): dense bright knots of gas joined
 * by luminous filaments — ridged noise thresholded soft, over a mottled
 * glowing base, several bright hotspots pulsing very slowly. */
#include "_gk336.h"

void pattern_367(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0014f;
    float hue0 = gk_sf(seed, 77) + t * 0.01f;
    float hx[4], hy[4];
    for (int i = 0; i < 4; i++) { hx[i] = 0.2f + 0.9f * gk_sf(seed, 80 + i); hy[i] = 0.2f + 0.6f * gk_sf(seed, 90 + i); }
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float fil = gk_ridge3(fx * 3.0f, fy * 3.0f, t * 0.3f, 4);
            float base = gk_fbm3(fx * 1.5f + 3.0f, fy * 1.5f, t * 0.2f, 3);
            float hot = 0;
            for (int i = 0; i < 4; i++) {
                float dx = fx - hx[i], dy = fy - hy[i];
                hot += expf(-(dx * dx + dy * dy) * 25.0f) * (0.7f + 0.3f * gk_sin(t * 1.5f + (float)i * 1.9f));
            }
            uint32_t bg = gk_pal(pal, hue0 + base * 0.2f);
            uint32_t f = gk_pal(pal, hue0 + 0.3f + fil * 0.1f);
            uint32_t hc = gk_lift(gk_pal(pal, hue0 + 0.55f), 0.35f);
            uint32_t c = gk_mix(bg, f, gk_sstep(0.3f, 0.85f, fil) * 0.8f);
            c = gk_mix(c, hc, gk_clamp01(hot));
            gk_put(y * GK_W + x, gk_shade(c, 0.55f + 0.3f * fil + 0.1f * base + 0.25f * gk_clamp01(hot)));
        }
    }
    gk_blit(fb, w, h);
}
