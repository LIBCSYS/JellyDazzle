/* pattern_417 — SUN HALO (ground): a bright disc with a 22-degree halo
 * ring, faint sundogs, and soft radial rays, over a gradient sky whose
 * top-to-bottom hues walk the ramp; the sun climbs and sinks over minutes. */
#include "_gk336.h"

void pattern_417(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 25) + t * 0.008f;
    float cx = GK_W * (0.5f + 0.2f * gk_sin(t * 0.3f)), cy = GK_H * (0.45f + 0.15f * gk_cos(t * 0.25f));
    float R = GK_H * 0.32f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float disc = expf(-r * r / (GK_H * GK_H * 0.006f));
            float halo = expf(-(r - R) * (r - R) / 90.0f) * 0.7f;
            float dogs = expf(-(r - R) * (r - R) / 200.0f) * (expf(-(a) * (a) * 8.0f) + expf(-(gk_absf(a) - 3.14159f) * (gk_absf(a) - 3.14159f) * 8.0f)) * 0.5f;
            float rays = (gk_n3(gk_cos(a) * 3.0f, gk_sin(a) * 3.0f, t * 0.4f) * 0.5f + 0.5f) * expf(-r * 0.006f) * 0.4f;
            uint32_t sky = gk_pal(pal, hue0 + fy * 0.25f);
            uint32_t sun = gk_lift(gk_pal(pal, hue0 + 0.4f), 0.5f);
            uint32_t haloc = gk_pal(pal, hue0 + 0.55f + (r - R) * 0.004f);
            uint32_t c = gk_mix(sky, haloc, gk_clamp01(halo + dogs));
            c = gk_mix(c, sun, gk_clamp01(disc + rays * 0.5f));
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.2f * (halo + dogs) + 0.3f * disc + 0.2f * rays));
        }
    }
    gk_blit(fb, w, h);
}
