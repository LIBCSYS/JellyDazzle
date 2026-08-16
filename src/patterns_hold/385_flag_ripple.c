/* pattern_385 — FLAG RIPPLE (ground): a coloured cloth rippling in a slow
 * wind — travelling waves along one axis with a standing wave across,
 * shaded from the wave slope; the cloth carries broad colour bands. */
#include "_gk336.h"

void pattern_385(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.004f;
    float hue0 = gk_sf(seed, 17) + t * 0.008f;
    int nb = 4 + (int)(gk_sf(seed, 18) * 4.0f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_W, fy = (float)y / GK_H;
            float amp = 0.3f + 0.7f * fx;
            float ph = fx * 9.0f - t * 1.5f + fy * 2.0f;
            float wv = gk_sin(ph) * amp + 0.4f * gk_sin(fx * 5.0f + t * 0.9f + fy * 4.0f) * gk_sin(fy * 3.14f);
            float slope = gk_cos(ph) * amp + 0.4f * gk_cos(fx * 5.0f + t * 0.9f + fy * 4.0f) * gk_sin(fy * 3.14f);
            float band = (fy + 0.05f * wv) * (float)nb;
            int bi = (int)floorf(band); float fr = band - (float)bi;
            float e = gk_sstep(0.4f, 0.6f, fr);
            uint32_t c = gk_mix(gk_pal(pal, hue0 + (float)bi * 0.11f), gk_pal(pal, hue0 + (float)(bi + 1) * 0.11f), e);
            gk_put(y * GK_W + x, gk_shade(c, 0.68f + 0.22f * slope + 0.05f * wv));
        }
    }
    gk_blit(fb, w, h);
}
