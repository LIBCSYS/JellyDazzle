/* pattern_433 — SHIBORI STRIPES (ground): resist-dyed stripes — bands
 * that wobble along their length with soft bleeding edges, some broad,
 * some fine, in the indigo-and-white rhythm but coloured from the ramp. */
#include "_gk336.h"

void pattern_433(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 59) + t * 0.008f;
    float ang = gk_sf(seed, 60) * 3.14f + 0.05f * gk_sin(t * 0.3f), ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            float wob = gk_n3(v * 0.02f, u * 0.004f, t * 0.3f) * 14.0f + gk_n3(v * 0.06f + 5.0f, u * 0.01f, t * 0.4f) * 4.0f;
            float uu = u + wob;
            float s1 = gk_sstep(-0.2f, 0.2f, gk_sin(uu * 0.05f));
            float s2 = gk_sstep(0.5f, 0.8f, gk_sin(uu * 0.2f + 1.0f)) * 0.5f;
            float dye = gk_clamp01(s1 * 0.8f + s2);
            float bleed = gk_fbm3((float)x * 0.03f, (float)y * 0.03f, t * 0.3f, 2) * 0.1f;
            uint32_t light = gk_lift(gk_pal(pal, hue0 + bleed), 0.35f);
            uint32_t deep = gk_pal(pal, hue0 + 0.4f + bleed);
            gk_put(y * GK_W + x, gk_shade(gk_mix(light, deep, dye), 0.75f + 0.15f * (1.0f - dye) + bleed));
        }
    }
    gk_blit(fb, w, h);
}
