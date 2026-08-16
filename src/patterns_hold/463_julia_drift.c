/* pattern_463 — JULIA DRIFT (ground): a Julia set with its parameter on
 * a slow orbit, coloured by smooth escape time through the ramp — but
 * capped low and blurred by the small canvas so it stays soft, not spiky. */
#define GK_W 256
#define GK_H 192
#include "_gk336.h"

void pattern_463(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0006f;
    float hue0 = gk_sf(seed, 29) + t * 0.008f;
    float ph = gk_sf(seed, 30) * 6.28f;
    float cr = -0.75f + 0.12f * gk_cos(t + ph), ci = 0.11f + 0.14f * gk_sin(t * 0.7f + ph);
    float zoom = 1.6f, rot = t * 0.15f, ca = gk_cos(rot), sa = gk_sin(rot);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float u = ((float)x - GK_W * 0.5f) / GK_H * zoom * 2.0f, v = ((float)y - GK_H * 0.5f) / GK_H * zoom * 2.0f;
            float zr = u * ca - v * sa, zi = u * sa + v * ca;
            int k = 0; const int K = 24;
            float m2 = 0;
            for (; k < K; k++) {
                float nr = zr * zr - zi * zi + cr, ni = 2.0f * zr * zi + ci;
                zr = nr; zi = ni; m2 = zr * zr + zi * zi;
                if (m2 > 16.0f) break;
            }
            float sm;
            if (k >= K) sm = (float)K;
            else sm = (float)k + 1.0f - logf(logf(m2 + 1.0f) * 0.5f + 1.0f) / 0.6931f;
            float f = sm / (float)K;                       /* 0..1 */
            float inside = k >= K ? 1.0f : 0.0f;
            uint32_t c = gk_pal(pal, hue0 + f * 0.6f + (u * u + v * v) * 0.08f);
            uint32_t in = gk_pal(pal, hue0 + 0.75f + gk_n3(u * 3.0f, v * 3.0f, t) * 0.05f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(c, in, inside), 0.65f + 0.3f * gk_sstep(0.0f, 0.6f, f) - 0.15f * inside));
        }
    }
    gk_soften();
    gk_blit(fb, w, h);
}
