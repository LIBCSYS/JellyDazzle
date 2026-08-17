/* 535 Electric Jellyfish — three jellyfish of lightning drift slowly
 * upward through the frame: each a soft luminous bell (a breathing dome
 * outline over a dim inner glow) trailing six tentacles that are thin
 * jagged discharges, swaying on their own pendulum phases and re-jagging
 * on slow crossfades, bright at the bell and thinning to another hue at
 * the tips.  A jelly fades out at the top and a new one fades in below.
 * Each jelly has its own hue, all drifting.  Sparse-to-figure overlay.
 * Repaint. */
#include "_trace509.h"

#define NJ535 3
#define NT535 6
#define P535 120
#define RISE535 900

static gk g535;
static gk_bolt b535[NJ535][NT535][2];
static int bi535[NJ535][NT535][2];
static uint32_t bs535 = 0xFFFFFFFFu;

void pattern_535(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g535, w, h);
    gk_clear(&g535);
    if (seed != bs535) { memset(bi535, 0xFF, sizeof bi535); bs535 = seed; }
    float cw = (float)g535.cw, ch = (float)g535.ch, sc = g535.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    for (int j = 0; j < NJ535; j++) {
        int ph = frame + j * (RISE535 / NJ535);
        int cyc = ph / RISE535;
        float u = (float)(ph - cyc * RISE535) / (float)RISE535;      /* 0 bottom .. 1 top */
        uint32_t hs = (uint32_t)cyc * 3299u + (uint32_t)j * 7127u + seed;
        float jx = cw * (0.2f + 0.6f * gk_hash(hs + 1u)) + cw * 0.06f * sinf(t * 0.004f + (float)j * 2.0f);
        float jy = ch * (1.15f - 1.35f * u);
        float env = gk_smooth(u / 0.12f) * (1.0f - gk_smooth((u - 0.85f) / 0.15f));
        if (env <= 0.0f) continue;
        float bw = cw * (0.09f + 0.04f * gk_hash(hs + 2u)) * sc / sc;
        float breath = 1.0f + 0.06f * sinf(t * 0.02f + (float)j * 1.7f);
        float bwx = bw * breath, bwy = bw * 0.7f / breath;
        int pi = base + (int)(gk_hash(hs + 3u) * 8000.0f);
        /* bell: dome outline + inner glow */
        float bc[3], bg[3];
        gk_col(pal, pi + 400, 0.35f, 0.6f * env, bc);
        gk_col(pal, pi + 1100, 0.05f, 0.40f * env, bg);
        float lx = 0.0f, ly = 0.0f;
        for (int i = 0; i <= 28; i++) {
            float a = GK_TAU * 0.5f * (float)i / 28.0f;
            float x = jx - cosf(a) * bwx, y = jy - sinf(a) * bwy;
            if (i > 0) gk_seg(&g535, lx, ly, x, y, bc, 1.2f * sc, 5.0f * sc, 0.6f);
            lx = x; ly = y;
        }
        gk_dot(&g535, jx, jy - bwy * 0.4f, bg, bwx * 0.6f, bwx * 1.1f, 0.6f);
        /* tentacles */
        for (int k = 0; k < NT535; k++) {
            float rx = jx - bwx * 0.9f + bwx * 1.8f * ((float)k + 0.5f) / (float)NT535;
            int P = P535 + k * 11;
            int tp = frame + k * 37 + j * 53;
            int idx = tp / P;
            float cu = (float)(tp - idx * P) / (float)P;
            int g = idx & 1;
            if (bi535[j][k][g] != idx) { gk_seed(&g535, seed ^ (uint32_t)(idx * 1223 + j * 331 + k * 47)); gk_bolt_gen(&g535, &b535[j][k][g], 0.0f, 0.0f, 0.0f, 1.0f, 0.10f, 5, 2, 0.25f); bi535[j][k][g] = idx; }
            if (bi535[j][k][g ^ 1] != idx - 1) { gk_seed(&g535, seed ^ (uint32_t)((idx - 1) * 1223 + j * 331 + k * 47)); gk_bolt_gen(&g535, &b535[j][k][g ^ 1], 0.0f, 0.0f, 0.0f, 1.0f, 0.10f, 5, 2, 0.25f); bi535[j][k][g ^ 1] = idx - 1; }
            float wa = gk_smooth(cu), wb = 1.0f - wa;
            float sway = 0.25f * sinf(t * 0.011f + (float)k * 0.9f + (float)j * 2.0f);
            float len = bwx * (1.6f + 0.6f * gk_hash(hs + 10u + (uint32_t)k));
            float c0[3], c1[3], h0[3], h1[3];
            gk_col(pal, pi + 300, 0.05f, 0.35f * env, h0);
            gk_col(pal, pi + 2000, 0.05f, 0.25f * env, h1);
            gk_col(pal, pi + 600, 0.45f, 0.55f * env, c0);
            gk_col(pal, pi + 2300, 0.3f, 0.4f * env, c1);
            for (int q = 0; q < 2; q++) {
                float wgt = q == 0 ? wa : wb;
                const gk_bolt *b = &b535[j][k][q == 0 ? g : g ^ 1];
                bx_draw_grad(&g535, b, rx, jy, sway, len, 1.0f, 2.0f, wgt, h0, h1, 0.4f, 1.4f * sc, 4.5f * sc, 0.5f);
                bx_draw_grad(&g535, b, rx, jy, sway, len, 1.0f, 2.0f, wgt, c0, c1, 0.4f, 0.6f * sc, 1.6f * sc, 0.25f);
            }
        }
    }
    gk_present(&g535, fb, w, h);
}
