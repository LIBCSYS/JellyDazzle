/* pattern_440 — GOUACHE SMUDGE (ground): opaque paint smeared with a
 * broad knife — long streaks of colour dragged in one slow-turning
 * direction, colours mixing where streaks overlap, thick and matte. */
#include "_gk336.h"

void pattern_440(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 21) + t * 0.008f;
    float ang = gk_sf(seed, 22) * 3.14f + 0.1f * gk_sin(t * 0.3f), ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            /* streaks: noise stretched along u */
            float s1 = gk_n3(u * 0.003f, v * 0.04f, t * 0.3f);
            float s2 = gk_n3(u * 0.008f + 5.0f, v * 0.09f, t * 0.4f);
            float s3 = gk_n3(u * 0.002f + 9.0f, v * 0.015f, t * 0.2f);
            uint32_t a = gk_pal(pal, hue0 + s3 * 0.2f);
            uint32_t b = gk_pal(pal, hue0 + 0.3f + s1 * 0.1f);
            uint32_t c = gk_pal(pal, hue0 + 0.6f);
            uint32_t m = gk_mix(a, b, gk_sstep(-0.3f, 0.3f, s1));
            m = gk_mix(m, c, gk_sstep(0.2f, 0.5f, s2) * 0.7f);
            float ridge = gk_absf(s2) * 0.15f;
            gk_put(y * GK_W + x, gk_shade(m, 0.72f + ridge + 0.1f * s3));
        }
    }
    gk_blit(fb, w, h);
}
