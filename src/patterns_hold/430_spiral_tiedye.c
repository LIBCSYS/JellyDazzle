/* pattern_430 — SPIRAL TIE-DYE (ground): the classic spiral tie-dye —
 * colour bands that spiral out from a centre, edges bled by noise, the
 * spiral tightening and loosening over minutes and the centre wandering. */
#include "_gk336.h"

void pattern_430(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 25) + t * 0.008f;
    float cx = GK_W * 0.5f + 30.0f * gk_sin(t * 0.3f), cy = GK_H * 0.5f + 20.0f * gk_cos(t * 0.4f);
    float arms = (float)(2 + (int)(gk_sf(seed, 26) * 3.0f));
    float tight = 0.05f + 0.02f * gk_sin(t * 0.4f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float bleed = gk_fbm3((float)x * 0.02f, (float)y * 0.02f, t * 0.3f, 3) * 1.2f;
            float ph = a * arms + r * tight + t + bleed;
            float band = gk_fract(ph / 6.2832f);                 /* 0..1 along a turn */
            /* several colour stops per turn, soft-edged */
            float stops = 5.0f;
            float sb = band * stops; int si = (int)floorf(sb); float fs = sb - (float)si;
            float e = gk_sstep(0.3f, 0.7f, fs);
            uint32_t c = gk_mix(gk_pal(pal, hue0 + (float)si / stops * 0.7f), gk_pal(pal, hue0 + (float)((si + 1) % 5) / stops * 0.7f), e);
            gk_put(y * GK_W + x, gk_shade(c, 0.75f + 0.15f * gk_sin(fs * 6.28f) + 0.1f * expf(-r * 0.01f)));
        }
    }
    gk_blit(fb, w, h);
}
