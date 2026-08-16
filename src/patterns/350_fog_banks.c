/* pattern_350 — FOG BANKS (ground): horizontal layers of mist sliding at
 * different speeds over a soft two-tone gradient. Each layer is a low-
 * frequency noise band; nearer layers are brighter and move faster. */
#include "_gk336.h"

void pattern_350(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0025f;
    float hue0 = gk_sf(seed, 9) + t * 0.01f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float mist = 0, hue = 0;
            for (int k = 0; k < 4; k++) {
                float sp = 0.15f + 0.12f * (float)k;
                float n = gk_n3(fx * (1.5f + 0.5f * k) + t * sp, fy * 6.0f + (float)k * 7.0f, t * 0.1f + (float)k);
                float m = gk_sstep(-0.3f, 0.5f, n) * (0.35f + 0.2f * (float)k);
                mist += m; hue += m * 0.05f * (float)k;
            }
            mist = gk_clamp01(mist * 0.6f);
            uint32_t back = gk_pal(pal, hue0 + fy * 0.2f);
            uint32_t fog = gk_pal(pal, hue0 + 0.35f + hue);
            uint32_t c = gk_mix(back, fog, mist);
            gk_put(y * GK_W + x, gk_shade(c, 0.62f + 0.33f * mist));
        }
    }
    gk_blit(fb, w, h);
}
