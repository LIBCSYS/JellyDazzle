/* pattern_362 — SUPERNOVA SHELLS (ground): concentric expanding shells of
 * gas around an off-centre point, each shell a noisy ring; shells drift
 * outward so slowly the eye reads a breathing rather than a blast. */
#include "_gk336.h"

void pattern_362(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 57) + t * 0.01f;
    float cx = GK_W * (0.35f + 0.3f * gk_sf(seed, 58)), cy = GK_H * (0.4f + 0.2f * gk_sf(seed, 59));
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy) / GK_H, a = atan2f(dy, dx);
            float n = gk_fbm3(dx * 0.01f, dy * 0.01f, t * 0.3f, 3);
            float rr = r + n * 0.08f;
            float shell = gk_sin(rr * 22.0f - t * 1.2f + gk_sin(a * 3.0f) * 0.4f) * 0.5f + 0.5f;
            shell = shell * shell * shell;
            float env = expf(-r * 1.2f);
            uint32_t bg = gk_pal(pal, hue0 + r * 0.3f + n * 0.06f);
            uint32_t sh = gk_pal(pal, hue0 + 0.4f + rr * 0.2f);
            uint32_t c = gk_mix(bg, sh, shell * (0.4f + 0.6f * env));
            gk_put(y * GK_W + x, gk_shade(c, 0.55f + 0.30f * shell + 0.15f * env + 0.08f * n));
        }
    }
    gk_blit(fb, w, h);
}
