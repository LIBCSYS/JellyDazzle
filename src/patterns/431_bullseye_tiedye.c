/* pattern_431 — BULLSEYE TIE-DYE (ground): concentric dye rings from two
 * or three fold centres, bled into each other with noise, each ring a
 * different ramp stop — a soft target field breathing in and out. */
#include "_gk336.h"

void pattern_431(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 29) + t * 0.008f;
    int nc = 2 + (int)(gk_sf(seed, 30) * 2.0f);
    float cx[3], cy[3];
    for (int i = 0; i < nc; i++) {
        cx[i] = GK_W * (0.25f + 0.5f * gk_sf(seed, 40 + i)) + 15.0f * gk_sin(t * 0.4f + (float)i);
        cy[i] = GK_H * (0.25f + 0.5f * gk_sf(seed, 50 + i)) + 12.0f * gk_cos(t * 0.3f + (float)i * 2.0f);
    }
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float bleed = gk_fbm3((float)x * 0.02f, (float)y * 0.02f, t * 0.3f, 3) * 12.0f;
            float dmin = 1e9f;
            for (int i = 0; i < nc; i++) {
                float dx = (float)x - cx[i], dy = (float)y - cy[i];
                float d = sqrtf(dx * dx + dy * dy);
                if (d < dmin) dmin = d;
            }
            float ph = (dmin + bleed) * 0.04f - t * 0.5f;
            float band = gk_fract(ph / 6.2832f) * 4.0f;
            int si = (int)floorf(band); float fs = band - (float)si;
            float e = gk_sstep(0.3f, 0.7f, fs);
            uint32_t c = gk_mix(gk_pal(pal, hue0 + (float)si * 0.18f), gk_pal(pal, hue0 + (float)((si + 1) & 3) * 0.18f), e);
            gk_put(y * GK_W + x, gk_shade(c, 0.75f + 0.15f * gk_sin(fs * 6.28f) + 0.1f * expf(-dmin * 0.01f)));
        }
    }
    gk_blit(fb, w, h);
}
