/* pattern_354 — ROLLING MIST (ground): mist that rolls diagonally across
 * the frame in slow waves; a low-frequency sine carrier modulated by noise
 * so the crests break up. Colour shifts across the crest. */
#include "_gk336.h"

void pattern_354(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0025f;
    float hue0 = gk_sf(seed, 25) + t * 0.01f;
    float ang = gk_sf(seed, 26) * 3.14f;
    float ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H, fy = (float)y / GK_H;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            float n = gk_fbm3(fx * 3.0f, fy * 3.0f, t * 0.3f, 3);
            float crest = gk_sin(u * 5.0f - t * 1.5f + n * 2.0f) * 0.5f + 0.5f;
            float crest2 = gk_sin(v * 3.5f + t * 0.9f - n * 1.5f) * 0.5f + 0.5f;
            float m = crest * 0.6f + crest2 * 0.4f;
            uint32_t c = gk_pal(pal, hue0 + m * 0.22f + n * 0.08f);
            gk_put(y * GK_W + x, gk_shade(c, 0.58f + 0.36f * m));
        }
    }
    gk_blit(fb, w, h);
}
