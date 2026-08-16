/* pattern_348 — DAWN CLOUDS (ground): a vertical sky gradient (two palette
 * stops, drifting) with slow fBm cloud banks lit from below, brighter and
 * warmer near the horizon line which itself rises and falls over minutes. */
#include "_gk336.h"

void pattern_348(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0020f;
    float hueA = gk_sf(seed, 1) + t * 0.01f, hueB = hueA + 0.28f + 0.1f * gk_sf(seed, 2);
    float horizon = 0.62f + 0.08f * gk_sin(t * 0.3f);
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float n = gk_fbm3(fx * 2.2f + t * 0.15f, fy * 3.5f, t * 0.25f, 4);
            float cloud = gk_sstep(-0.15f, 0.35f, n + (fy - 0.4f) * 0.4f);
            float sky = gk_sstep(horizon + 0.25f, horizon - 0.35f, fy);
            uint32_t ca = gk_pal(pal, hueA + fy * 0.12f), cb = gk_pal(pal, hueB + n * 0.1f);
            uint32_t c = gk_mix(ca, cb, cloud * 0.8f);
            float lit = 0.62f + 0.30f * (1.0f - gk_absf(fy - horizon) * 1.6f) + 0.15f * cloud * (1.0f - fy);
            gk_put(y * GK_W + x, gk_shade(c, lit + 0.12f * sky));
        }
    }
    gk_blit(fb, w, h);
}
