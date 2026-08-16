/* pattern_384 — CORDUROY WAVES (ground): ribbed cloth — soft parallel
 * ridges (a rounded triangle wave) that bend along a slow wave, lit so
 * each rib has a light and a shadow side; the rib direction rotates. */
#include "_gk336.h"

void pattern_384(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 13) + t * 0.01f;
    float ang = t * 0.1f + gk_sf(seed, 14) * 3.0f, ca = gk_cos(ang), sa = gk_sin(ang);
    float per = 0.35f + 0.2f * gk_sf(seed, 15);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            float bend = 20.0f * gk_sin(v * 0.02f + t) + 10.0f * gk_n3(u * 0.01f, v * 0.01f, t * 0.3f);
            float ph = (u + bend) * per;
            float rib = gk_sin(ph), ribd = gk_cos(ph);
            float body = gk_n3(fx * 0.006f, fy * 0.006f, t * 0.2f);
            uint32_t c = gk_pal(pal, hue0 + body * 0.3f + rib * 0.03f);
            gk_put(y * GK_W + x, gk_shade(c, 0.72f + 0.14f * rib + 0.12f * ribd + 0.06f * body));
        }
    }
    gk_blit(fb, w, h);
}
