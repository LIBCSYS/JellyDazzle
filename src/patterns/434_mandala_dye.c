/* pattern_434 — MANDALA DYE (ground): a fold-dyed mandala — the frame is
 * folded 8 ways in angle so a single noisy dye field becomes a symmetric
 * bloom, bands of colour radiating and bleeding, turning slowly. */
#include "_gk336.h"

void pattern_434(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 63) + t * 0.008f;
    int folds = 6 + 2 * (int)(gk_sf(seed, 64) * 3.0f);
    float wseg = 6.2832f / (float)folds;
    float cx = GK_W * 0.5f, cy = GK_H * 0.5f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx) + t * 0.3f;
            float fa = fmodf(a + 20.0f * wseg, wseg);
            if (fa > wseg * 0.5f) fa = wseg - fa;
            float u = r * gk_cos(fa), v = r * gk_sin(fa);
            float dye = gk_fbm3(u * 0.015f, v * 0.015f, t * 0.3f, 3);
            float ph = r * 0.05f + dye * 3.0f - t;
            float band = gk_fract(ph / 6.2832f) * 4.0f;
            int si = (int)floorf(band); float fs = band - (float)si;
            float e = gk_sstep(0.3f, 0.7f, fs);
            uint32_t c = gk_mix(gk_pal(pal, hue0 + (float)si * 0.17f + dye * 0.05f), gk_pal(pal, hue0 + (float)((si + 1) & 3) * 0.17f + dye * 0.05f), e);
            gk_put(y * GK_W + x, gk_shade(c, 0.72f + 0.15f * gk_sin(fs * 6.28f) + 0.1f * expf(-r * 0.01f) + 0.05f * dye));
        }
    }
    gk_blit(fb, w, h);
}
