/* pattern_347 — CURL PLASMA (ground): the plasma phase is advected along a
 * curl-noise flow, so its bands stretch into slow eddies rather than
 * marching in straight lines. */
#include "_gk336.h"

void pattern_347(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0022f;
    float hue0 = gk_sf(seed, 121) + t * 0.02f;
    float ns = 0.008f + 0.004f * gk_sf(seed, 122);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            /* curl of a scalar noise: (dN/dy, -dN/dx) by finite differences */
            float e = 1.5f;
            float n1 = gk_n3(fx * ns, (fy + e) * ns, t * 0.4f), n2 = gk_n3(fx * ns, (fy - e) * ns, t * 0.4f);
            float n3 = gk_n3((fx + e) * ns, fy * ns, t * 0.4f), n4 = gk_n3((fx - e) * ns, fy * ns, t * 0.4f);
            float cx = (n1 - n2) * 40.0f, cy = -(n3 - n4) * 40.0f;
            float px = fx + cx * 8.0f, py = fy + cy * 8.0f;
            float c = gk_sin(px * 0.03f + t) + gk_sin(py * 0.028f - t * 0.7f) + gk_sin((px + py) * 0.018f + t * 0.5f);
            float v = 0.72f + 0.24f * gk_sin(c * 1.2f + t * 0.4f);
            gk_pix(y * GK_W + x, pal, hue0 + c * 0.08f, v);
        }
    }
    gk_blit(fb, w, h);
}
