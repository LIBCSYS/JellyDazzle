/* pattern_357 — TWILIGHT GLOW (ground): a large soft glow low in the frame
 * (the sun just gone) with faint radiating haze, over a gradient that
 * deepens upward. The glow centre and its width drift over minutes. */
#include "_gk336.h"

void pattern_357(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 37) + t * 0.008f;
    float gx = 0.5f + 0.25f * gk_sin(t * 0.4f), gy = 0.78f + 0.08f * gk_sin(t * 0.27f);
    float wid = 0.5f + 0.15f * gk_sin(t * 0.33f + 1.0f);
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float dx = fx - gx * ((float)GK_W / GK_H), dy = fy - gy;
            float d = sqrtf(dx * dx + dy * dy * 1.6f);
            float glow = expf(-d * d / (wid * wid));
            float a = atan2f(dy, dx);
            float rays = 0.5f + 0.5f * gk_sin(a * 7.0f + t * 0.5f) * gk_sin(a * 3.0f - t * 0.3f);
            float haze = gk_fbm3(fx * 2.0f, fy * 2.0f, t * 0.25f, 3) * 0.5f + 0.5f;
            float hue = hue0 + fy * 0.2f + haze * 0.05f + rays * 0.03f;
            uint32_t c = gk_mix(gk_pal(pal, hue), gk_pal(pal, hue0 + 0.42f + d * 0.3f), glow);
            float lit = 0.55f + 0.40f * glow + 0.10f * rays * (1.0f - glow) * (1.0f - fy) + 0.05f * haze;
            gk_put(y * GK_W + x, gk_shade(c, lit));
        }
    }
    gk_blit(fb, w, h);
}
