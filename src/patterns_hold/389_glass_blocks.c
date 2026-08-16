/* pattern_389 — GLASS BLOCKS (ground): a Mondrian-ish wall of rectangular
 * glass panes — a recursive-looking split grid (jittered rows and columns),
 * each pane a colour, light sweeping across the wall so panes brighten in
 * turn. */
#include "_gk336.h"

void pattern_389(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 33) + t * 0.008f;
    /* jittered column and row edges */
    static float ce[16], re[16]; static uint32_t last = 0xFFFFFFFFu;
    int nc = 7, nr = 5;
    if (last != seed) {
        last = seed;
        for (int i = 0; i <= nc; i++) ce[i] = ((float)i + 0.35f * (gk_sf(seed, 40 + i) - 0.5f) * (i > 0 && i < nc)) / nc * GK_W;
        for (int i = 0; i <= nr; i++) re[i] = ((float)i + 0.35f * (gk_sf(seed, 60 + i) - 0.5f) * (i > 0 && i < nr)) / nr * GK_H;
    }
    float sweep = GK_W * (0.5f + 0.7f * gk_sin(t * 0.7f));
    for (int y = 0; y < GK_H; y++) {
        int rj = 0; while (rj < nr - 1 && (float)y >= re[rj + 1]) rj++;
        float ry0 = re[rj], ry1 = re[rj + 1];
        for (int x = 0; x < GK_W; x++) {
            int ci = 0; while (ci < nc - 1 && (float)x >= ce[ci + 1]) ci++;
            float cx0 = ce[ci], cx1 = ce[ci + 1];
            /* some columns merge with the row above to make tall panes */
            uint32_t hh = gk_hash2(ci, rj, seed);
            int merge = (hh & 7) == 0 && rj > 0;
            uint32_t pid = merge ? gk_hash2(ci, rj - 1, seed) : hh;
            float ex = gk_sstep(0.0f, 3.0f, (float)x - cx0) * gk_sstep(0.0f, 3.0f, cx1 - (float)x);
            float ey = merge ? gk_sstep(0.0f, 3.0f, ry1 - (float)y) : gk_sstep(0.0f, 3.0f, (float)y - ry0) * gk_sstep(0.0f, 3.0f, ry1 - (float)y);
            float lead = ex * ey;
            float grad = ((float)x - cx0) / (cx1 - cx0 + 1.0f) * 0.5f + ((float)y - ry0) / (ry1 - ry0 + 1.0f) * 0.5f;
            float lit = expf(-((float)x - sweep) * ((float)x - sweep) * 0.00003f);
            uint32_t pane = gk_pal(pal, hue0 + gk_hf(pid) * 0.6f + grad * 0.04f);
            uint32_t frame_ = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.4f);
            uint32_t c = gk_mix(frame_, pane, lead);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.15f * grad + 0.25f * lit));
        }
    }
    gk_blit(fb, w, h);
}
