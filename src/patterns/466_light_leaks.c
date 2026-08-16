/* pattern_466 — LIGHT LEAKS (ground): film light leaks — broad diagonal
 * bands of warm light bleeding across a graded field, with a soft flare
 * disc and faint streaks, drifting slowly as if the film crept. */
#include "_gk336.h"

void pattern_466(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 49) + t * 0.008f;
    float ang = 0.6f + 0.4f * gk_sf(seed, 50), ca = gk_cos(ang), sa = gk_sin(ang);
    float fxp = GK_W * (0.5f + 0.35f * gk_sin(t * 0.3f)), fyp = GK_H * (0.5f + 0.3f * gk_cos(t * 0.4f));
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x;
            float u = fx * ca + (float)y * sa;
            float leak = 0;
            for (int k = 0; k < 3; k++) {
                float c0 = 120.0f * (float)k + 80.0f * gk_sin(t * (0.3f + 0.1f * k) + (float)k * 2.0f);
                float wdt = 40.0f + 25.0f * gk_sf(seed, 60 + k);
                leak += expf(-(u - c0) * (u - c0) / (wdt * wdt)) * (0.5f + 0.5f * gk_sf(seed, 70 + k));
            }
            float dx = fx - fxp, dy = (float)y - fyp;
            float flare = expf(-(dx * dx + dy * dy) * 0.0002f);
            float streak = gk_n2(u * 0.05f, 3.0f) * 0.5f + 0.5f;
            uint32_t base = gk_pal(pal, hue0 + fy * 0.2f);
            uint32_t warm = gk_pal(pal, hue0 + 0.4f + leak * 0.05f);
            uint32_t c = gk_mix(base, warm, gk_clamp01(leak * 0.8f));
            c = gk_mix(c, gk_lift(gk_pal(pal, hue0 + 0.55f), 0.4f), flare * 0.7f);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.25f * gk_clamp01(leak) + 0.25f * flare + 0.05f * streak));
        }
    }
    gk_blit(fb, w, h);
}
