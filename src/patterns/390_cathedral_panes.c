/* pattern_390 — CATHEDRAL PANES (ground): tall lancet windows side by side,
 * each filled with a slow vertical gradient of coloured glass and a
 * pointed arch top; the light behind them drifts, so panes glow in turn. */
#include "_gk336.h"

void pattern_390(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 45) + t * 0.008f;
    int np = 5 + (int)(gk_sf(seed, 46) * 3.0f);
    float pw = (float)GK_W / np;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float px = (float)x / pw; int pi = (int)floorf(px); float fr = px - (float)pi;
            float u = (fr - 0.5f) * 2.0f;                       /* -1..1 across pane */
            /* pointed arch: pane top y = 0.12 + 0.18*|u|^1.5 (arch) */
            float au = gk_absf(u);
            float top = 0.10f + 0.22f * au * sqrtf(au);
            float inside = gk_sstep(top - 0.02f, top + 0.02f, fy) * gk_sstep(1.0f, 0.97f, fy) * gk_sstep(0.0f, 0.06f, 1.0f - au);
            /* mullion down the middle and horizontal bars */
            float bars = gk_sstep(0.0f, 0.03f, gk_absf(u)) * (0.6f + 0.4f * gk_sstep(0.0f, 0.02f, gk_absf(gk_fract(fy * 4.0f) - 0.5f) - 0.0f));
            float glass = inside * bars;
            uint32_t hh = gk_hash2(pi, (int)floorf(fy * 4.0f), seed);
            float glow = 0.5f + 0.5f * gk_sin(t * 1.5f + (float)pi * 1.1f + fy * 2.0f);
            uint32_t pane = gk_pal(pal, hue0 + gk_hf(hh) * 0.35f + fy * 0.1f);
            uint32_t stone = gk_pal(pal, hue0 + 0.5f + gk_n2((float)x * 0.02f, (float)y * 0.02f) * 0.03f);
            uint32_t c = gk_mix(gk_shade(stone, 0.55f), pane, glass);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.3f * glow * glass + 0.1f * (1.0f - fy)));
        }
    }
    gk_blit(fb, w, h);
}
