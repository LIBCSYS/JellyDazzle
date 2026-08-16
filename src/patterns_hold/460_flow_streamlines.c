/* pattern_460 — FLOW STREAMLINES (ground): a smooth vector field made
 * visible — noise smeared along the flow (a short line-integral trace per
 * pixel), so the frame reads as flowing hair or river weed, drifting. */
#define GK_W 192
#define GK_H 144
#include "_gk336.h"

void pattern_460(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 17) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float px = (float)x, py = (float)y, acc = 0, wsum = 0;
            /* trace forward and back along the field, sampling a fine noise */
            for (int dir = -1; dir <= 1; dir += 2) {
                float qx = px, qy = py;
                for (int k = 0; k < 5; k++) {
                    float ang = gk_n3(qx * 0.008f, qy * 0.008f, t * 0.3f) * 3.14159f * 1.5f;
                    qx += gk_cos(ang) * 2.0f * (float)dir; qy += gk_sin(ang) * 2.0f * (float)dir;
                    float wgt = 1.0f - (float)k / 6.0f;
                    acc += gk_n2(qx * 0.12f, qy * 0.12f) * wgt; wsum += wgt;
                }
            }
            float lic = acc / wsum;                          /* -0.5..0.5 ish */
            float big = gk_fbm3(px * 0.006f, py * 0.006f, t * 0.2f, 3);
            uint32_t c = gk_pal(pal, hue0 + big * 0.3f + lic * 0.15f);
            gk_put(y * GK_W + x, gk_shade(c, 0.72f + 0.35f * lic + 0.05f * big));
        }
    }
    gk_blit(fb, w, h);
}
