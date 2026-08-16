/* pattern_459 — SOFT KALEIDO (ground): a noise field folded 6 or 8 ways
 * about the centre — a slow kaleidoscope of soft colour blobs, no hard
 * mirror seams (the fold is on a smooth field), turning imperceptibly. */
#include "_gk336.h"

void pattern_459(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 13) + t * 0.008f;
    int folds = 6 + 2 * (int)(gk_sf(seed, 14) * 3.0f);
    float wseg = 6.2832f / (float)folds;
    float cx = GK_W * 0.5f, cy = GK_H * 0.5f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx) + t * 0.2f;
            float fa = fmodf(a + 20.0f * wseg, wseg);
            if (fa > wseg * 0.5f) fa = wseg - fa;
            float u = r * gk_cos(fa) * 0.012f, v = r * gk_sin(fa) * 0.012f;
            float n1 = gk_fbm3(u + t * 0.3f, v, t * 0.25f, 3);
            float n2 = gk_fbm3(u * 1.7f + 9.0f, v * 1.7f - t * 0.2f, t * 0.3f, 3);
            uint32_t c = gk_pal(pal, hue0 + n1 * 0.35f + n2 * 0.15f + r * 0.0005f);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.25f * (n2 * 0.5f + 0.5f) + 0.15f * expf(-r * 0.01f)));
        }
    }
    gk_blit(fb, w, h);
}
