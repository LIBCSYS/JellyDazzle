/* pattern_351 — RAIN VEIL (ground): soft vertical streaks — many faint
 * columns of varying brightness that slide downward at different rates —
 * over a gradient sky, like rain seen against distant light. */
#include "_gk336.h"

void pattern_351(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.003f;
    float hue0 = gk_sf(seed, 13) + t * 0.006f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float s1 = gk_n2(fx * 5.0f, fy * 0.6f + t * 0.8f);
            float s2 = gk_n2(fx * 11.0f + 40.0f, fy * 1.0f + t * 1.4f);
            float s3 = gk_n2(fx * 2.5f + 80.0f, fy * 0.3f + t * 0.4f);
            float streak = 0.5f + 0.25f * s1 + 0.12f * s2 + 0.25f * s3;
            float glow = gk_n3(fx * 1.5f, fy * 1.5f, t * 0.2f);
            uint32_t c = gk_pal(pal, hue0 + fy * 0.15f + glow * 0.1f + streak * 0.12f);
            gk_put(y * GK_W + x, gk_shade(c, 0.62f + 0.35f * streak));
        }
    }
    gk_blit(fb, w, h);
}
