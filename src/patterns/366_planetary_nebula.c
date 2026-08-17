/* pattern_366 — PLANETARY NEBULA (ground): a bright ring nebula — an
 * elliptical shell with a soft rim, an inner glow and outer halo, textured
 * with noise, all slowly rotating and breathing in size. */
#include "_gk336.h"

void pattern_366(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0014f;
    float hue0 = gk_sf(seed, 73) + t * 0.01f;
    float rot = t * 0.3f, cr = gk_cos(rot), sr = gk_sin(rot);
    float ecc = 0.65f + 0.2f * gk_sf(seed, 74);
    float R = 0.30f + 0.03f * gk_sin(t * 0.7f);
    float cx = GK_W * 0.5f, cy = GK_H * 0.5f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = ((float)x - cx) / GK_H, dy = ((float)y - cy) / GK_H;
            float u = dx * cr - dy * sr, v = (dx * sr + dy * cr) / ecc;
            float n = gk_fbm3(dx * 4.0f, dy * 4.0f, t * 0.4f, 3);
            float r = sqrtf(u * u + v * v) + n * 0.04f;
            float shell = expf(-(r - R) * (r - R) * 250.0f);
            float inner = expf(-r * r * 12.0f) * 0.7f;
            float halo = expf(-r * 2.0f) * 0.5f;
            uint32_t bg = gk_pal(pal, hue0 + r * 0.25f + n * 0.05f);
            uint32_t sh = gk_pal(pal, hue0 + 0.35f + n * 0.08f);
            uint32_t in = gk_pal(pal, hue0 + 0.6f);
            uint32_t c = gk_mix(bg, sh, shell);
            c = gk_mix(c, in, inner);
            gk_put(y * GK_W + x, gk_shade(c, 0.5f + 0.4f * shell + 0.3f * inner + 0.2f * halo + 0.08f * n));
        }
    }
    gk_blit(fb, w, h);
}
