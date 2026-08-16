/* 522 Ribbon Wave — two long horizontal discharges span the whole width,
 * each a jagged channel riding a slow travelling wave, so the ribbon
 * undulates across the frame while its jag morphs gradually (noise-
 * driven, never re-rolled); a persistence canvas keeps a translucent
 * ribbon of fading copies behind each channel's motion.  Hue slides along
 * each ribbon and the two ribbons carry different hues; both drift with
 * time.  Field-density overlay across the middle band.  Repaint with
 * memory (decay). */
#include "_trace509.h"

#define NP522 130

static gk g522, g522b;   /* persistent soft ribbon; fresh copy for the live channels */
static uint32_t bs522 = 0xFFFFFFFFu;

void pattern_522(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g522, w, h);
    if (seed != bs522) { gk_clear(&g522); bs522 = seed; }
    gk_decay_snap(&g522, 0.90f);
    gk_setup(&g522b, w, h);
    memcpy(g522b.acc, g522.acc, sizeof(float) * (size_t)(g522.cw * g522.ch) * 3);
    float cw = (float)g522.cw, ch = (float)g522.ch, sc = g522.sc, t = (float)frame;
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    for (int r = 0; r < 2; r++) {
        float yc = ch * (r == 0 ? 0.42f : 0.58f) + ch * 0.04f * sinf(t * 0.003f + (float)r * 2.0f);
        float amp = ch * (0.10f + 0.05f * gk_noise1(t * 0.004f, 50u + (uint32_t)r + seed));
        float kx = GK_TAU * (1.5f + 0.5f * (float)r) / cw;
        float om = 0.012f * (r == 0 ? 1.0f : -0.8f);
        int pi = base + r * 3800;
        float env = 0.85f + 0.15f * sinf(t * 0.02f + (float)r);
        float lx = 0.0f, ly = 0.0f;
        for (int i = 0; i < NP522; i++) {
            float u = (float)i / (float)(NP522 - 1);
            float x = u * cw;
            float y = yc + amp * sinf(kx * x - om * t) * (0.6f + 0.4f * sinf(u * 4.0f + t * 0.005f));
            /* jag: two-stream noise per point so it morphs slowly */
            float j = (gk_noise1(t * 0.008f + (float)i * 1.7f, 700u + (uint32_t)r + seed) - 0.5f) * 3.0f * sc
                    + (gk_hash((uint32_t)i * 37u + (uint32_t)r * 501u + seed) - 0.5f) * 12.0f * sc;
            y += j;
            if (i > 0) {
                int pj = pi + (int)(u * 4500.0f);
                float hc[3], c[3], rc[3];
                gk_col(pal, pj + 1400, 0.05f, 0.05f * env, rc);      /* accumulates ~10x on the persistent canvas */
                gk_seg(&g522, lx, ly, x, y, rc, 3.0f * sc, 9.0f * sc, 0.8f);
                gk_col(pal, pj + 700, 0.05f, 0.30f * env, hc);
                gk_col(pal, pj, 0.5f, 0.55f * env, c);
                gk_seg(&g522b, lx, ly, x, y, hc, 1.8f * sc, 6.0f * sc, 0.5f);
                gk_seg(&g522b, lx, ly, x, y, c, 0.8f * sc, 2.0f * sc, 0.25f);
            }
            lx = x; ly = y;
        }
    }
    gk_present(&g522b, fb, w, h);
}
