/* pattern_396 — BYZANTINE TESSERAE (ground): small square tesserae laid
 * in flowing rows that follow curved courses (opus vermiculatum), gold
 * ground with coloured bands, each tessera slightly tilted so it glints. */
#include "_gk336.h"

void pattern_396(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 17) + t * 0.008f;
    float cell = 14.0f;
    float lx = gk_cos(t * 0.7f), ly = gk_sin(t * 0.7f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            /* curved courses: warp coordinates by big noise */
            float wx = gk_n2(fx * 0.004f, fy * 0.004f) * 30.0f, wy = gk_n2(fx * 0.004f + 7.0f, fy * 0.004f) * 30.0f;
            float u = fx + wx, v = fy + wy;
            int ui = (int)floorf(u / cell), vi = (int)floorf(v / cell);
            float pu = gk_fract(u / cell) - 0.5f, pv = gk_fract(v / cell) - 0.5f;
            float edge = gk_sstep(0.5f, 0.38f, gk_absf(pu)) * gk_sstep(0.5f, 0.38f, gk_absf(pv));
            uint32_t hh = gk_hash2(ui, vi, seed);
            float tilt = (gk_hf(hh) - 0.5f) * lx + (gk_hf(hh >> 7) - 0.5f) * ly;
            float band = gk_n3(u * 0.006f, v * 0.006f, t * 0.3f);
            float isband = gk_sstep(0.1f, 0.3f, band);
            uint32_t gold = gk_pal(pal, hue0 + gk_hf(hh >> 11) * 0.05f + band * 0.05f);
            uint32_t col = gk_pal(pal, hue0 + 0.3f + band * 0.3f);
            uint32_t tess = gk_mix(gold, col, isband);
            uint32_t grout = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.5f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(grout, tess, edge), 0.78f + 0.2f * tilt + 0.05f * band));
        }
    }
    gk_blit(fb, w, h);
}
