/* pattern_379 — RAKED GARDEN (ground): a zen garden — concentric raked
 * rings around two or three stones, straight raked lines elsewhere, the
 * grooves soft, the pattern turning imperceptibly. */
#include "_gk336.h"

void pattern_379(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 53) + t * 0.01f;
    int ns = 2 + (int)(gk_sf(seed, 54) * 2.0f);
    float sx[3], sy[3], sr[3];
    for (int i = 0; i < ns; i++) {
        sx[i] = GK_W * (0.2f + 0.6f * gk_sf(seed, 60 + i)) + 10.0f * gk_sin(t + (float)i);
        sy[i] = GK_H * (0.2f + 0.6f * gk_sf(seed, 70 + i)) + 8.0f * gk_cos(t * 0.7f + (float)i);
        sr[i] = 14.0f + 16.0f * gk_sf(seed, 80 + i);
    }
    float ang = t * 0.15f, ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float ph = (fx * ca + fy * sa) * 0.35f;         /* straight rake */
            float wgt = 1.0f, stone = 0.0f;
            for (int i = 0; i < ns; i++) {
                float dx = fx - sx[i], dy = fy - sy[i], r = sqrtf(dx * dx + dy * dy);
                float inf = expf(-(r - sr[i]) * 0.02f); if (inf > 1.0f) inf = 1.0f;
                ph = ph * (1.0f - inf) + (r * 0.35f) * inf;
                stone += gk_sstep(sr[i] + 3.0f, sr[i] - 3.0f, r);
                wgt *= 1.0f;
            }
            (void)wgt;
            float groove = gk_sin(ph) * 0.5f + 0.5f;
            float grain = gk_fbm2(fx * 0.1f, fy * 0.1f, 2) * 0.06f;
            uint32_t sand = gk_pal(pal, hue0 + groove * 0.06f + grain);
            uint32_t rock = gk_pal(pal, hue0 + 0.4f);
            uint32_t c = gk_mix(sand, rock, gk_clamp01(stone));
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.3f * groove + grain - 0.15f * gk_clamp01(stone)));
        }
    }
    gk_blit(fb, w, h);
}
