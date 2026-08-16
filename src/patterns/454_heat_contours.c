/* pattern_454 — HEAT CONTOURS (ground): a thermal map — a few hot
 * spots blooming and cooling on staggered timers over a warm-to-cool
 * ramp, drawn with soft isotherms; nothing jumps, spots take a minute. */
#include "_gk336.h"

void pattern_454(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 33) + t * 0.008f;
    float hx[5], hy[5], hp[5];
    for (int i = 0; i < 5; i++) {
        hx[i] = GK_W * (0.15f + 0.7f * gk_sf(seed, 40 + i)) + 20.0f * gk_sin(t * 0.3f + (float)i);
        hy[i] = GK_H * (0.15f + 0.7f * gk_sf(seed, 50 + i)) + 15.0f * gk_cos(t * 0.4f + (float)i * 2.0f);
        hp[i] = 0.5f + 0.5f * gk_sin(t * (0.5f + 0.15f * i) + gk_sf(seed, 60 + i) * 6.28f);
    }
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float heat = gk_fbm3((float)x * 0.005f, (float)y * 0.005f, t * 0.2f, 3) * 0.2f + 0.2f;
            for (int i = 0; i < 5; i++) {
                float dx = (float)x - hx[i], dy = (float)y - hy[i];
                heat += hp[i] * expf(-(dx * dx + dy * dy) * 0.0003f);
            }
            heat = gk_clamp01(heat);
            float lv = heat * 10.0f, fr = gk_fract(lv);
            float line = 1.0f - gk_sstep(0.0f, 0.15f, fr) * gk_sstep(1.0f, 0.85f, fr);
            uint32_t c = gk_pal(pal, hue0 + heat * 0.45f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(c, gk_lift(c, 0.4f), line * 0.5f), 0.6f + 0.35f * heat));
        }
    }
    gk_blit(fb, w, h);
}
