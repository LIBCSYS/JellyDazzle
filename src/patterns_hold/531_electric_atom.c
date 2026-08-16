/* 531 Electric Atom — a glowing nucleus at the centre and three electrons
 * circling it on tilted elliptical orbits (each orbit a faint luminous
 * ring, tilted at a different angle, all turning very slowly), every
 * electron dragging a jagged filament back to the nucleus that re-forms
 * on a slow crossfade.  Each orbit has its own hue; the filaments run from
 * the nucleus hue to the electron hue; all drift with time.  Figure
 * overlay, black between.  Repaint. */
#include "_trace509.h"

#define NE531 3
#define P531 130

static gk g531;
static gk_bolt b531[NE531][2];
static int bi531[NE531][2];
static uint32_t bs531 = 0xFFFFFFFFu;

void pattern_531(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g531, w, h);
    gk_clear(&g531);
    if (seed != bs531) { for (int e = 0; e < NE531; e++) bi531[e][0] = bi531[e][1] = -1; bs531 = seed; }
    float cw = (float)g531.cw, ch = (float)g531.ch, sc = g531.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float cx = cw * 0.5f, cy = ch * 0.5f;
    float R = (cw < ch ? cw : ch) * 0.40f;
    for (int e = 0; e < NE531; e++) {
        float tilt = GK_TAU * (float)e / (float)NE531 + t * 0.0009f;
        float ecc = 0.32f + 0.05f * sinf(t * 0.003f + (float)e);
        float ct = cosf(tilt), st = sinf(tilt);
        int pi = base + e * 2700;
        /* orbit ring, faint */
        float oc[3];
        gk_col(pal, pi + 900, 0.1f, 0.14f, oc);
        float lx = 0.0f, ly = 0.0f;
        for (int i = 0; i <= 64; i++) {
            float a = GK_TAU * (float)i / 64.0f;
            float ox = cosf(a) * R, oy = sinf(a) * R * ecc;
            float x = cx + ox * ct - oy * st, y = cy + ox * st + oy * ct;
            if (i > 0) gk_seg(&g531, lx, ly, x, y, oc, 0.8f * sc, 3.0f * sc, 0.5f);
            lx = x; ly = y;
        }
        /* electron */
        float a = t * (0.009f + 0.002f * (float)e) * (e & 1 ? -1.0f : 1.0f) + (float)e * 2.0f;
        float ox = cosf(a) * R, oy = sinf(a) * R * ecc;
        float ex = cx + ox * ct - oy * st, ey = cy + ox * st + oy * ct;
        /* filament to the nucleus, crossfading */
        int ph = frame + e * (P531 / NE531);
        int idx = ph / P531;
        float u = (float)(ph - idx * P531) / (float)P531;
        int j = idx & 1;
        if (bi531[e][j] != idx) { gk_seed(&g531, seed ^ (uint32_t)(idx * 2833 + e * 919)); gk_bolt_gen(&g531, &b531[e][j], 0.0f, 0.0f, 1.0f, 0.0f, 0.15f, 6, 2, 0.3f); bi531[e][j] = idx; }
        if (bi531[e][j ^ 1] != idx - 1) { gk_seed(&g531, seed ^ (uint32_t)((idx - 1) * 2833 + e * 919)); gk_bolt_gen(&g531, &b531[e][j ^ 1], 0.0f, 0.0f, 1.0f, 0.0f, 0.15f, 6, 2, 0.3f); bi531[e][j ^ 1] = idx - 1; }
        float wa = gk_smooth(u), wb = 1.0f - wa;
        float dx = ex - cx, dy = ey - cy, len = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
        float br = 0.75f + 0.25f * gk_noise1(t * 0.02f + (float)e * 5.0f, 60u + (uint32_t)e);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, base, 0.05f, 0.35f * br, h0);
        gk_col(pal, pi + 1500, 0.05f, 0.40f * br, h1);
        gk_col(pal, base + 300, 0.5f, 0.55f * br, c0);
        gk_col(pal, pi + 1800, 0.5f, 0.65f * br, c1);
        for (int k = 0; k < 2; k++) {
            float wgt = k == 0 ? wa : wb;
            const gk_bolt *b = &b531[e][k == 0 ? j : j ^ 1];
            bx_draw_grad(&g531, b, cx, cy, ang, len, 1.0f, 2.0f, wgt, h0, h1, 0.0f, 1.6f * sc, 5.5f * sc, 0.5f);
            bx_draw_grad(&g531, b, cx, cy, ang, len, 1.0f, 2.0f, wgt, c0, c1, 0.0f, 0.7f * sc, 1.9f * sc, 0.25f);
        }
        float k0[3], k1[3];
        gk_col(pal, pi + 1800, 0.5f, 0.9f, k0);
        gk_col(pal, pi + 2200, 0.1f, 0.35f, k1);
        gk_dot(&g531, ex, ey, k1, 6.0f * sc, 18.0f * sc, 0.6f);
        gk_dot(&g531, ex, ey, k0, 2.5f * sc, 8.0f * sc, 0.6f);
    }
    float n0[3], n1[3], n2[3];
    gk_col(pal, base, 0.55f, 1.0f, n0);
    gk_col(pal, base + 700, 0.2f, 0.4f, n1);
    gk_col(pal, base + 1400, 0.05f, 0.12f, n2);
    gk_dot(&g531, cx, cy, n2, 20.0f * sc, 45.0f * sc, 0.5f);
    gk_dot(&g531, cx, cy, n1, 8.0f * sc, 20.0f * sc, 0.6f);
    gk_dot(&g531, cx, cy, n0, 4.0f * sc, 10.0f * sc, 0.6f);
    gk_present(&g531, fb, w, h);
}
