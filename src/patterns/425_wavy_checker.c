/* pattern_425 — WAVY CHECKER (ground): a checkerboard seen through
 * rippling glass — sine-warped, cells alternately two hue families that
 * drift along the ramp, edges soft, the ripple travelling slowly. */
#include "_gk336.h"

void pattern_425(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0022f;
    float hue0 = gk_sf(seed, 5) + t * 0.008f;
    float cell = 30.0f + 12.0f * gk_sf(seed, 6);
    float rot = 0.2f + 0.05f * gk_sin(t * 0.3f), cr = gk_cos(rot), sr = gk_sin(rot);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u0 = fx * cr - fy * sr, v0 = fx * sr + fy * cr;
            float u = u0 + 18.0f * gk_sin(v0 * 0.03f + t) + 8.0f * gk_sin(v0 * 0.07f - t * 0.6f);
            float v = v0 + 18.0f * gk_sin(u0 * 0.025f - t * 0.8f) + 8.0f * gk_sin(u0 * 0.06f + t * 0.5f);
            float su = gk_sin(u * 6.2832f / cell), sv = gk_sin(v * 6.2832f / cell);
            float chk = gk_sstep(-0.35f, 0.35f, su * sv);
            float wave = gk_n3(fx * 0.004f, fy * 0.004f, t * 0.2f);
            uint32_t a = gk_pal(pal, hue0 + wave * 0.15f), b = gk_pal(pal, hue0 + 0.4f + wave * 0.15f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(a, b, chk), 0.65f + 0.15f * gk_absf(su * sv) + 0.15f * wave + 0.05f));
        }
    }
    gk_blit(fb, w, h);
}
