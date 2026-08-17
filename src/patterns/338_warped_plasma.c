/* pattern_338 — WARPED PLASMA (ground): the plasma coordinates are pushed
 * through a slow fBm domain warp before the sines see them, so the bands
 * bend and swirl like oil on water instead of running straight. */
#include "_gk336.h"

void pattern_338(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0030f;
    float hue0 = gk_sf(seed, 11) + t * 0.02f;
    float amp = 18.0f + 14.0f * gk_sf(seed, 12);
    float ns = 0.011f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float wx = gk_n3(fx * ns, fy * ns, t * 0.35f) * amp;
            float wy = gk_n3(fx * ns + 31.0f, fy * ns + 17.0f, t * 0.35f + 5.0f) * amp;
            float px = fx + wx, py = fy + wy;
            float c = gk_sin(px * 0.030f + t) + gk_sin(py * 0.026f - t * 0.8f)
                    + gk_sin((px + py) * 0.020f + t * 0.5f);
            float v = 0.72f + 0.24f * gk_sin(c * 1.4f + t * 0.3f);
            gk_pix(y * GK_W + x, pal, hue0 + c * 0.08f, v);
        }
    }
    gk_blit(fb, w, h);
}
