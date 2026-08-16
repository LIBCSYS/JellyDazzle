/* pattern_406 — VELVET SWIRL (ground): pile swirled in slow spirals — the
 * nap direction follows a vortex field around two wandering centres, and
 * the highlight is where the nap faces the light: soft luminous spirals. */
#include "_gk336.h"

void pattern_406(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 9) + t * 0.008f;
    float ax = GK_W * (0.5f + 0.3f * gk_sin(t * 0.4f)), ay = GK_H * (0.5f + 0.3f * gk_cos(t * 0.3f));
    float bx = GK_W * (0.5f + 0.35f * gk_cos(t * 0.27f + 2.0f)), by = GK_H * (0.5f + 0.35f * gk_sin(t * 0.36f + 1.0f));
    float lx = gk_cos(t * 0.8f), ly = gk_sin(t * 0.8f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float d1x = fx - ax, d1y = fy - ay, r1 = d1x * d1x + d1y * d1y + 3000.0f;
            float d2x = fx - bx, d2y = fy - by, r2 = d2x * d2x + d2y * d2y + 3000.0f;
            /* vortex velocities (perpendicular) */
            float vx = -d1y / r1 * 3000.0f + d2y / r2 * 3000.0f, vy = d1x / r1 * 3000.0f - d2x / r2 * 3000.0f;
            float nl = 1.0f / (sqrtf(vx * vx + vy * vy) + 1.5f); vx *= nl; vy *= nl;
            float nap = vx * lx + vy * ly;
            float sheen = gk_clamp01(nap * 0.5f + 0.5f); sheen *= sheen;
            float body = gk_n3(fx * 0.005f, fy * 0.005f, t * 0.2f);
            float fine = gk_n2(fx * 0.02f, fy * 0.02f) * 0.03f;
            uint32_t deep = gk_pal(pal, hue0 + body * 0.15f + fine);
            uint32_t lit = gk_lift(gk_pal(pal, hue0 + 0.12f + nap * 0.04f), 0.2f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(deep, lit, sheen), 0.55f + 0.4f * sheen + fine));
        }
    }
    gk_blit(fb, w, h);
}
