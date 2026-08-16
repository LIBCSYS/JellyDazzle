/* pattern_374 — GOD RAYS (ground): underwater light shafts — a fan of soft
 * bright rays from a point above the frame, sliding sideways as the surface
 * moves, over a depth gradient with drifting particulate haze. */
#include "_gk336.h"

void pattern_374(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0025f;
    float hue0 = gk_sf(seed, 25) + t * 0.01f;
    float sx = GK_W * (0.5f + 0.3f * gk_sin(t * 0.3f)), sy = -GK_H * 0.4f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - sx, dy = (float)y - sy;
            float a = atan2f(dx, dy);
            float rays = gk_n2(a * 9.0f + t * 0.5f, 1.0f) * 0.5f + gk_n2(a * 23.0f - t * 0.8f, 5.0f) * 0.3f + gk_n2(a * 4.0f + t * 0.2f, 9.0f) * 0.4f;
            rays = gk_sstep(-0.3f, 0.6f, rays);
            float depth = 1.0f - fy;
            float haze = gk_fbm3((float)x * 0.01f, (float)y * 0.01f - t * 0.3f, t * 0.2f, 3);
            uint32_t deep = gk_pal(pal, hue0 + fy * 0.2f + haze * 0.05f);
            uint32_t ray = gk_pal(pal, hue0 + 0.35f + a * 0.05f);
            uint32_t c = gk_mix(deep, ray, rays * depth * 0.7f);
            gk_put(y * GK_W + x, gk_shade(c, 0.55f + 0.35f * rays * depth + 0.1f * haze + 0.1f * depth));
        }
    }
    gk_blit(fb, w, h);
}
