/* pattern_365 — DARK NEBULA (ground): a bright luminous field with dark
 * dust silhouettes drifting in front of it — the reverse of the usual
 * nebula. The silhouettes are soft-edged and never fully black. */
#include "_gk336.h"

void pattern_365(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 69) + t * 0.01f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float light = gk_fbm3(fx * 1.6f, fy * 1.6f, t * 0.3f, 3);
            float dust = gk_fbm3(fx * 2.8f + t * 0.1f + 20.0f, fy * 2.8f, t * 0.35f, 4);
            float sil = gk_sstep(0.05f, 0.4f, dust);
            uint32_t bright = gk_pal(pal, hue0 + light * 0.2f + fy * 0.05f);
            uint32_t dark = gk_pal(pal, hue0 + 0.45f + dust * 0.05f);
            uint32_t c = gk_mix(bright, dark, sil);
            gk_put(y * GK_W + x, gk_shade(c, (0.75f + 0.25f * light) * (1.0f - 0.45f * sil)));
        }
    }
    gk_blit(fb, w, h);
}
