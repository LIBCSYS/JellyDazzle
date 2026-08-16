/* pattern_404 — CRUSHED VELVET (ground): pile pressed in random
 * directions — a cell field where each patch has its own nap direction,
 * so a moving light makes patches flare and dim independently, soft-edged. */
#include "_gk336.h"

void pattern_404(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 1) + t * 0.008f;
    float lx = gk_cos(t * 0.6f), ly = gk_sin(t * 0.6f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float ax = gk_n3(fx * 0.006f, fy * 0.006f, t * 0.15f), ay = gk_n3(fx * 0.006f + 9.0f, fy * 0.006f + 4.0f, t * 0.15f);
            float nl = 1.0f / (sqrtf(ax * ax + ay * ay) + 0.25f); ax *= nl; ay *= nl;
            float nap = ax * lx + ay * ly;                       /* -1..1 */
            float sheen = gk_clamp01(nap * 0.5f + 0.5f);
            sheen = sheen * sheen;
            float fine = gk_n2(fx * 0.03f, fy * 0.03f) * 0.03f;
            float body = gk_n3(fx * 0.004f, fy * 0.004f, t * 0.2f);
            uint32_t deep = gk_pal(pal, hue0 + body * 0.25f + fine);
            uint32_t lit = gk_lift(gk_pal(pal, hue0 + 0.12f + body * 0.1f), 0.2f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(deep, lit, sheen), 0.55f + 0.4f * sheen + fine));
        }
    }
    gk_blit(fb, w, h);
}
