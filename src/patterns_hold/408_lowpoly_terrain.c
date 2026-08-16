/* pattern_408 — LOW-POLY TERRAIN (ground): a faceted landscape — a
 * triangulated height field seen from above, each triangle flat-shaded
 * by its slope against a slowly circling sun; colour by height. */
#include "_gk336.h"

void pattern_408(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 17) + t * 0.008f;
    float S = 30.0f;
    float lx = gk_cos(t * 0.5f), ly = gk_sin(t * 0.5f);
    #define HT(I, J) (gk_n3((float)(I) * 0.35f, (float)(J) * 0.35f, t * 0.5f) + 0.5f * gk_n3((float)(I) * 0.9f + 3.0f, (float)(J) * 0.9f, t * 0.3f))
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float u = (float)x / S, v = (float)y / (S * 0.866f);
            int vi = (int)floorf(v); float fv = v - (float)vi;
            float us = u - (vi & 1) * 0.5f;
            int ui = (int)floorf(us); float fu = us - (float)ui;
            int upper = fu + fv > 1.0f;
            /* triangle vertices in lattice space */
            int i0 = ui, j0 = vi, i1 = ui + 1, j1 = vi, i2 = ui + ((vi & 1) ? 1 : 0), j2 = vi + 1;
            if (upper) { i0 = ui + 1; j0 = vi + 1; i1 = ui + ((vi & 1) ? 1 : 0); j1 = vi + 1; i2 = ui + 1; j2 = vi; }
            float h0 = HT(i0, j0), h1 = HT(i1, j1), h2 = HT(i2, j2);
            float hm = (h0 + h1 + h2) * 0.3333f;
            /* slope from vertex heights */
            float gx = (h1 - h0), gy = (h2 - h0) * (upper ? -1.0f : 1.0f);
            float diff = gk_clamp01(0.55f + (gx * lx + gy * ly) * 0.9f);
            uint32_t c = gk_pal(pal, hue0 + hm * 0.25f);
            gk_put(y * GK_W + x, gk_shade(c, 0.5f + 0.45f * diff + 0.05f * hm));
        }
    }
    #undef HT
    gk_blit(fb, w, h);
}
