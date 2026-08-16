/* pattern_438 — COLOUR FIELD WASHES (ground): Rothko-like — a few broad
 * horizontal fields of colour with soft, slightly ragged edges, each
 * field's hue drifting round the ramp at its own pace, faint mottle within. */
#include "_gk336.h"

void pattern_438(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.001f;
    float hue0 = gk_sf(seed, 13) + t * 0.008f;
    int nf = 3 + (int)(gk_sf(seed, 14) * 2.0f);
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_W;
            float rag = gk_fbm3(fx * 6.0f, fy * 6.0f, t * 0.4f, 3) * 0.04f;
            float p = (fy + rag) * (float)nf;
            int fi = (int)floorf(p); float fr = p - (float)fi;
            float e = gk_sstep(0.42f, 0.58f, fr);
            float mott = gk_fbm3(fx * 3.0f + (float)fi * 5.0f, fy * 3.0f, t * 0.3f, 3);
            float ha = hue0 + (float)fi * 0.24f + gk_sin(t + (float)fi) * 0.05f, hb = hue0 + (float)(fi + 1) * 0.24f + gk_sin(t + (float)fi + 1.0f) * 0.05f;
            uint32_t c = gk_mix(gk_pal(pal, ha + mott * 0.03f), gk_pal(pal, hb + mott * 0.03f), e);
            float margin = gk_sstep(0.0f, 0.06f, fx) * gk_sstep(1.0f, 0.94f, fx);
            uint32_t bg = gk_pal(pal, hue0 + 0.5f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(bg, c, 0.3f + 0.7f * margin), 0.72f + 0.12f * mott + 0.1f * margin));
        }
    }
    gk_blit(fb, w, h);
}
