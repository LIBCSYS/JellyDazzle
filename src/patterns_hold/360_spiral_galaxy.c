/* pattern_360 — SPIRAL GALAXY (ground): two soft logarithmic arms wound
 * about a bright core, textured with noise, the whole thing turning at a
 * few degrees a minute; the field between the arms is filled with faint
 * gas so nothing is black. */
#include "_gk336.h"

void pattern_360(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 49) + t * 0.01f;
    float wind = 2.5f + 1.5f * gk_sf(seed, 50);
    float cx = GK_W * 0.5f, cy = GK_H * 0.5f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = ((float)y - cy) * 1.25f;
            float r = sqrtf(dx * dx + dy * dy) / GK_H, a = atan2f(dy, dx);
            float n = gk_fbm3(dx * 0.012f, dy * 0.012f, t * 0.4f, 3);
            float arm = gk_sin(a * 2.0f - logf(r + 0.02f) * wind + t * 1.5f + n * 1.2f) * 0.5f + 0.5f;
            arm = arm * arm * arm;
            float core = expf(-r * r * 30.0f);
            float fade = expf(-r * 1.0f);
            uint32_t gas = gk_pal(pal, hue0 + r * 0.2f + n * 0.05f);
            uint32_t armc = gk_lift(gk_pal(pal, hue0 + 0.4f + n * 0.1f), 0.2f);
            uint32_t c = gk_mix(gas, armc, arm * fade);
            c = gk_mix(c, gk_lift(gk_pal(pal, hue0 + 0.5f), 0.4f), core);
            gk_put(y * GK_W + x, gk_shade(c, 0.5f + 0.45f * arm * fade + 0.1f * fade + 0.3f * core + 0.1f * n));
        }
    }
    gk_blit(fb, w, h);
}
