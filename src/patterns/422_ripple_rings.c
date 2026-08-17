/* pattern_422 — RIPPLE RINGS (ground): several ring systems from points
 * that drift slowly, summed as heights and shaded like a surface, so
 * they merge into a soft interference relief rather than hard circles. */
#include "_gk336.h"

void pattern_422(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 45) + t * 0.008f;
    float sx[4], sy[4];
    for (int i = 0; i < 4; i++) {
        float ph = gk_sf(seed, 50 + i) * 6.28f;
        sx[i] = GK_W * (0.5f + 0.4f * gk_sin(t * (0.2f + 0.07f * i) + ph));
        sy[i] = GK_H * (0.5f + 0.4f * gk_cos(t * (0.17f + 0.06f * i) + ph * 1.3f));
    }
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float hgt = 0, gx = 0, gy = 0;
            for (int i = 0; i < 4; i++) {
                float dx = (float)x - sx[i], dy = (float)y - sy[i], r = sqrtf(dx * dx + dy * dy) + 0.01f;
                float k = 0.08f + 0.01f * (float)i;
                float env = expf(-r * 0.006f);
                float s = gk_sin(r * k - t * 2.0f), c = gk_cos(r * k - t * 2.0f);
                hgt += s * env; gx += c * k * env * dx / r; gy += c * k * env * dy / r;
            }
            float lit = gk_clamp01(0.55f + (gx * gk_cos(t) + gy * gk_sin(t)) * 2.5f);
            uint32_t c = gk_pal(pal, hue0 + hgt * 0.08f + (float)y * 0.0004f);
            gk_put(y * GK_W + x, gk_shade(c, 0.5f + 0.4f * lit + 0.1f * hgt));
        }
    }
    gk_blit(fb, w, h);
}
