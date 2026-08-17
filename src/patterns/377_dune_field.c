/* pattern_377 — DUNE FIELD (ground): big soft dunes — a low-frequency
 * ridged height field shaded by a slow-moving sun so crest lines glow and
 * slip faces fall into colour shadow. */
#include "_gk336.h"

void pattern_377(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 45) + t * 0.01f;
    float lx = gk_cos(t * 0.4f), ly = gk_sin(t * 0.4f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x * 0.006f, fy = (float)y * 0.006f;
            #define HD(X, Y) (1.0f - gk_absf(gk_n3((X), (Y) * 1.6f, t * 0.15f)) + 0.3f * gk_n3((X) * 3.0f + 5.0f, (Y) * 3.0f, t * 0.2f))
            float e = 0.02f;
            float h0 = HD(fx, fy);
            float dxh = HD(fx + e, fy) - h0, dyh = HD(fx, fy + e) - h0;
            #undef HD
            float lit = gk_clamp01(0.5f + (dxh * lx + dyh * ly) * 20.0f);
            float grain = gk_fbm2((float)x * 0.03f, (float)y * 0.03f, 2) * 0.03f;
            uint32_t c = gk_pal(pal, hue0 + h0 * 0.15f + lit * 0.08f + grain);
            gk_put(y * GK_W + x, gk_shade(c, 0.45f + 0.4f * lit + 0.15f * h0 + grain));
        }
    }
    gk_blit(fb, w, h);
}
