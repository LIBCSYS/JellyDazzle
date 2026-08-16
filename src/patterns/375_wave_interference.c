/* pattern_375 — WAVE INTERFERENCE (ground): circular waves from three slow
 * moving sources summed — the classic ripple-tank moire, but with long
 * wavelengths and soft crests so it reads as breathing rings, not stripes. */
#include "_gk336.h"

void pattern_375(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0025f;
    float hue0 = gk_sf(seed, 29) + t * 0.01f;
    float sx[3], sy[3];
    for (int i = 0; i < 3; i++) {
        float ph = gk_sf(seed, 30 + i) * 6.28f;
        sx[i] = GK_W * (0.5f + 0.35f * gk_sin(t * (0.3f + 0.1f * i) + ph));
        sy[i] = GK_H * (0.5f + 0.35f * gk_cos(t * (0.25f + 0.08f * i) + ph * 1.7f));
    }
    float k = 0.05f + 0.02f * gk_sf(seed, 35);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float sum = 0;
            for (int i = 0; i < 3; i++) {
                float dx = (float)x - sx[i], dy = (float)y - sy[i];
                sum += gk_sin(sqrtf(dx * dx + dy * dy) * k - t * 2.0f + (float)i);
            }
            sum /= 3.0f;
            float v = sum * 0.5f + 0.5f;
            uint32_t c = gk_pal(pal, hue0 + v * 0.3f + (float)y * 0.0003f);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.36f * v));
        }
    }
    gk_blit(fb, w, h);
}
