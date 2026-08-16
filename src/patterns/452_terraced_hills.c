/* pattern_452 — TERRACED HILLS (ground): filled contour terraces — the
 * height field quantised into soft-edged steps, each step a ramp stop
 * with a lit riser on the sun side; the sun circles very slowly. */
#include "_gk336.h"

void pattern_452(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0008f;
    float hue0 = gk_sf(seed, 25) + t * 0.008f;
    float lx = gk_cos(t * 0.6f), ly = gk_sin(t * 0.6f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x * 0.005f, fy = (float)y * 0.005f;
            float hgt = gk_fbm3(fx, fy, t * 0.3f, 3) * 0.5f + 0.5f;
            float e = 0.01f;
            float gx = gk_fbm3(fx + e, fy, t * 0.3f, 3) * 0.5f + 0.5f - hgt;
            float gy = gk_fbm3(fx, fy + e, t * 0.3f, 3) * 0.5f + 0.5f - hgt;
            float lv = hgt * 9.0f;
            float step = floorf(lv), fr = lv - step;
            float ease = gk_sstep(0.85f, 1.0f, fr);              /* soft riser */
            float lvl = (step + ease) / 9.0f;
            float riser = gk_sstep(0.8f, 0.95f, fr) * (1.0f - gk_sstep(0.95f, 1.0f, fr));
            float sun = gk_clamp01(0.5f + (gx * lx + gy * ly) * 30.0f);
            uint32_t c = gk_pal(pal, hue0 + lvl * 0.45f);
            gk_put(y * GK_W + x, gk_shade(c, 0.65f + 0.2f * lvl + 0.25f * riser * (sun - 0.5f) + 0.05f * fr));
        }
    }
    gk_blit(fb, w, h);
}
