/* pattern_416 — FAN BLADES (ground): overlapping translucent fan blades
 * — a few wide petal-shaped lobes from a centre, each rotating at its own
 * slow rate in its own tint, blended where they overlap. */
#include "_gk336.h"

void pattern_416(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 17) + t * 0.008f;
    float cx = GK_W * 0.5f + 40.0f * gk_sin(t * 0.3f), cy = GK_H * 0.5f + 30.0f * gk_cos(t * 0.4f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float rs = 0, gs = 0, bs = 0, ws = 0.001f;
            for (int k = 0; k < 4; k++) {
                float sp = 0.5f + 0.3f * (float)k, ph = gk_sf(seed, 20 + k) * 6.28f;
                float lobes = 3.0f + (float)k;
                float f = gk_sin(a * lobes + t * sp * (k & 1 ? -1.0f : 1.0f) + ph) * 0.5f + 0.5f;
                f = f * f * f;
                float wgt = f * (0.4f + 0.6f * expf(-r * 0.004f * (float)(k + 1)));
                uint32_t c = gk_pal(pal, hue0 + (float)k * 0.18f + r * 0.0004f);
                rs += ((c >> 16) & 255) * wgt; gs += ((c >> 8) & 255) * wgt; bs += (c & 255) * wgt; ws += wgt;
            }
            uint32_t base = gk_pal(pal, hue0 + 0.6f + r * 0.0005f);
            float lit = 0.6f + 0.35f * gk_clamp01(ws * 0.9f);
            float mixw = gk_clamp01(ws * 1.2f);
            float br = ((base >> 16) & 255) * (1.0f - mixw) + rs / ws * mixw;
            float bg = ((base >> 8) & 255) * (1.0f - mixw) + gs / ws * mixw;
            float bb = (base & 255) * (1.0f - mixw) + bs / ws * mixw;
            gk_putf(y * GK_W + x, br / 255.0f * lit, bg / 255.0f * lit, bb / 255.0f * lit);
        }
    }
    gk_blit(fb, w, h);
}
