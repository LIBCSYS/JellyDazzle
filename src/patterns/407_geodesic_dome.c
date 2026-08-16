/* pattern_407 — GEODESIC DOME (ground): the inside of a geodesic dome —
 * a triangular lattice with soft struts, each facet flat-shaded from a
 * slowly turning light, colours drifting across the facets in waves. */
#include "_gk336.h"

void pattern_407(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 13) + t * 0.008f;
    float S = 34.0f + 10.0f * gk_sf(seed, 14);
    float ang = t * 0.05f, ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            /* dome bulge: fisheye the coordinates */
            float rr = sqrtf(fx * fx + fy * fy) / (GK_H * 0.9f);
            float mag = 1.0f + rr * rr * 0.6f;
            float u0 = (fx * ca + fy * sa) * mag, v0 = (-fx * sa + fy * ca) * mag;
            /* triangular lattice */
            float u = u0 / S, v = v0 / (S * 0.866f);
            int vi = (int)floorf(v); float fv = v - (float)vi;
            float us = u - (vi & 1) * 0.5f;
            int ui = (int)floorf(us); float fu = us - (float)ui;
            int upper = fu + fv > 1.0f;   /* which triangle */
            /* barycentric edge distance for struts */
            float e1 = fv, e2 = fu, e3 = 1.0f - fu - fv;
            float ed = upper ? fminf(fminf(1.0f - e1, 1.0f - e2), -e3) : fminf(fminf(e1, e2), e3);
            float strut = gk_sstep(0.0f, 0.08f, ed);
            uint32_t hh = gk_hash2(ui * 2 + upper, vi, seed);
            float wave = gk_n3((float)ui * 0.2f, (float)vi * 0.2f, t * 0.4f);
            float lit = 0.5f + 0.4f * gk_sin(t * 1.5f + gk_hf(hh) * 6.28f) * 0.5f + 0.2f * (upper ? 1.0f : -1.0f) * gk_sin(t * 0.7f);
            uint32_t facet = gk_pal(pal, hue0 + wave * 0.3f + gk_hf(hh) * 0.03f);
            uint32_t frame_ = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.55f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(frame_, facet, strut), 0.6f + 0.35f * lit + 0.05f * (1.0f - rr)));
        }
    }
    gk_blit(fb, w, h);
}
