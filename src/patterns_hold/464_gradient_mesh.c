/* pattern_464 — GRADIENT MESH (ground): a 4x3 mesh of colour control
 * points, each drifting round the ramp on its own clock, smoothly
 * interpolated across the frame — the calmest thing in the library. */
#include "_gk336.h"

void pattern_464(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 33) + t * 0.006f;
    uint32_t cp[3][4];
    for (int j = 0; j < 3; j++) for (int i = 0; i < 4; i++) {
        float ph = gk_sf(seed, 40 + j * 4 + i) * 6.28f;
        cp[j][i] = gk_pal(pal, hue0 + gk_sf(seed, 60 + j * 4 + i) * 0.7f + 0.06f * gk_sin(t + ph));
    }
    for (int y = 0; y < GK_H; y++) {
        float v = (float)y / GK_H * 2.0f; int vj = (int)v; if (vj > 1) vj = 1; float fv = gk_fade(v - (float)vj);
        for (int x = 0; x < GK_W; x++) {
            float u = (float)x / GK_W * 3.0f; int ui = (int)u; if (ui > 2) ui = 2; float fu = gk_fade(u - (float)ui);
            uint32_t a = gk_mix(cp[vj][ui], cp[vj][ui + 1], fu), b = gk_mix(cp[vj + 1][ui], cp[vj + 1][ui + 1], fu);
            uint32_t c = gk_mix(a, b, fv);
            float n = gk_n3((float)x * 0.006f, (float)y * 0.006f, t * 0.3f);
            gk_put(y * GK_W + x, gk_shade(c, 0.72f + 0.12f * n));
        }
    }
    gk_blit(fb, w, h);
}
