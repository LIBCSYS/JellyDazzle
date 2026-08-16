/* pattern_394 — HEX MOSAIC (ground): honeycomb tiles with soft seams,
 * coloured by a slow travelling wave across the lattice plus a per-tile
 * jitter; a broad glow crosses the field so tiles light up in turn. */
#include "_gk336.h"

void pattern_394(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 9) + t * 0.008f;
    float R = 16.0f + 8.0f * gk_sf(seed, 10);
    float gx = GK_W * (0.5f + 0.5f * gk_sin(t * 0.5f)), gy = GK_H * (0.5f + 0.5f * gk_cos(t * 0.33f));
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            /* axial hex coords */
            float q = (0.57735f * fx - fy / 3.0f) / R, r = (fy * 2.0f / 3.0f) / R;
            float cq = roundf(q), cr = roundf(r), cs = roundf(-q - r);
            float dq = gk_absf(cq - q), dr = gk_absf(cr - r), ds = gk_absf(cs + q + r);
            if (dq > dr && dq > ds) cq = -cr - cs; else if (dr > ds) cr = -cq - cs;
            float cxp = R * 1.73205f * (cq + cr * 0.5f), cyp = R * 1.5f * cr;
            float dx = fx - cxp, dy = fy - cyp;
            /* hex distance */
            float ax = gk_absf(dx), ay = gk_absf(dy);
            float hd = fmaxf(ax, ax * 0.5f + ay * 0.866f) / (R * 0.866f);   /* 1 at edge */
            float seam = gk_sstep(1.0f, 0.88f, hd);
            uint32_t hh = gk_hash2((int)cq, (int)cr, seed);
            float wave = gk_n3(cq * 0.12f, cr * 0.12f, t * 0.4f);
            float ddx = cxp - gx, ddy = cyp - gy;
            float glow = expf(-(ddx * ddx + ddy * ddy) * 0.00004f);
            uint32_t tile = gk_pal(pal, hue0 + wave * 0.3f + gk_hf(hh) * 0.04f + glow * 0.1f);
            uint32_t seamc = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.55f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(seamc, tile, seam), 0.65f + 0.3f * glow + 0.05f * (1.0f - hd)));
        }
    }
    gk_blit(fb, w, h);
}
