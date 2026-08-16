/* pattern_415 — RAYS THROUGH CLOUD (ground): a sunburst seen through a
 * drifting cloud layer — the rays are modulated by fBm so they flicker
 * slowly in and out, and the cloud takes the ray colour where lit. */
#include "_gk336.h"

void pattern_415(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 13) + t * 0.008f;
    float cx = GK_W * (0.5f + 0.3f * gk_sin(t * 0.3f)), cy = GK_H * (0.3f + 0.15f * gk_cos(t * 0.2f));
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float ca = gk_cos(a), sa = gk_sin(a);
            float rays = gk_n3(ca * 2.5f, sa * 2.5f, t * 0.3f) * 0.5f + gk_n3(ca * 6.0f + 9.0f, sa * 6.0f, t * 0.5f) * 0.3f + 0.5f;
            rays = gk_sstep(0.2f, 0.9f, rays);
            float cloud = gk_fbm3((float)x * 0.008f + t * 0.2f, (float)y * 0.008f, t * 0.3f, 4) * 0.5f + 0.5f;
            float glow = expf(-r * 0.006f);
            uint32_t sky = gk_pal(pal, hue0 + cloud * 0.2f + (float)y * 0.0005f);
            uint32_t ray = gk_pal(pal, hue0 + 0.4f + ca * 0.03f);
            uint32_t c = gk_mix(sky, ray, rays * (0.4f + 0.6f * glow) * (1.0f - cloud * 0.5f));
            gk_put(y * GK_W + x, gk_shade(c, 0.55f + 0.25f * rays * (0.5f + glow) + 0.15f * cloud + 0.1f * glow));
        }
    }
    gk_blit(fb, w, h);
}
