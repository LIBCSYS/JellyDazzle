/* 519 Plasma Frame — the frame itself is the plasma globe: a bright
 * electrode at the centre throws ten slow filaments that reach for the
 * rectangular edge of the picture, each attach point sliding along the
 * border on its own noise clock, each filament re-jagging on a slow
 * crossfade between two geometries so it seems to swim; where a filament
 * touches, the border glows.  Filaments are coloured root-to-tip with two
 * palette hues that differ per filament and drift with time.  Figure
 * overlay.  Repaint. */
#include "_trace509.h"

#define NF519 10
#define P519 170

static gk g519;
static gk_bolt b519[NF519][2];
static int bi519[NF519][2];
static uint32_t bs519 = 0xFFFFFFFFu;

static void gen519(int f, int j, int idx, uint32_t seed)
{
    gk_seed(&g519, seed ^ (uint32_t)(idx * 7027 + f * 2113 + j * 883));
    gk_bolt_gen(&g519, &b519[f][j], 0.04f, 0.0f, 1.0f, 0.0f, 0.13f, 6, 2, 0.3f);
    bi519[f][j] = idx;
}

/* perimeter position u (0..1) -> point on the rectangle inset by m */
static void rim519(float u, float cw, float ch, float m, float *x, float *y)
{
    float pw = cw - 2.0f * m, ph = ch - 2.0f * m, per = 2.0f * (pw + ph);
    float d = u - floorf(u); d *= per;
    if (d < pw) { *x = m + d; *y = m; return; } d -= pw;
    if (d < ph) { *x = cw - m; *y = m + d; return; } d -= ph;
    if (d < pw) { *x = cw - m - d; *y = ch - m; return; } d -= pw;
    *x = m; *y = ch - m - d;
}

void pattern_519(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g519, w, h);
    gk_clear(&g519);
    if (seed != bs519) { for (int f = 0; f < NF519; f++) bi519[f][0] = bi519[f][1] = -1; bs519 = seed; }
    float cw = (float)g519.cw, ch = (float)g519.ch, sc = g519.sc, t = (float)frame;
    float cx = cw * 0.5f + cw * 0.02f * sinf(t * 0.004f), cy = ch * 0.5f + ch * 0.02f * cosf(t * 0.0033f);
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    float m = 6.0f * sc;
    /* the border, faint */
    float bc[3];
    gk_col(pal, base + 4000, 0.1f, 0.10f, bc);
    gk_seg(&g519, m, m, cw - m, m, bc, 0.8f * sc, 3.0f * sc, 0.4f);
    gk_seg(&g519, cw - m, m, cw - m, ch - m, bc, 0.8f * sc, 3.0f * sc, 0.4f);
    gk_seg(&g519, cw - m, ch - m, m, ch - m, bc, 0.8f * sc, 3.0f * sc, 0.4f);
    gk_seg(&g519, m, ch - m, m, m, bc, 0.8f * sc, 3.0f * sc, 0.4f);
    for (int f = 0; f < NF519; f++) {
        int ph = frame + f * (P519 * 2 / NF519);
        int idx = ph / P519;
        float u = (float)(ph - idx * P519) / (float)P519;      /* crossfade phase */
        int j = idx & 1;
        if (bi519[f][j] != idx) gen519(f, j, idx, seed);
        if (bi519[f][j ^ 1] != idx - 1 && bi519[f][j ^ 1] != idx + 1) gen519(f, j ^ 1, idx - 1, seed);
        float wa = gk_smooth(u), wb = 1.0f - wa;               /* j fades in, j^1 fades out */
        float pu = (float)f / (float)NF519 + 0.08f * gk_noise1(t * 0.0025f + (float)f * 3.1f, 400u + (uint32_t)f + seed) + t * 0.00004f;
        float ex, ey;
        rim519(pu, cw, ch, m, &ex, &ey);
        float dx = ex - cx, dy = ey - cy, len = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
        int pi = base + f * 800;
        float br = 0.75f + 0.25f * gk_noise1(t * 0.02f + (float)f * 7.7f, 900u + (uint32_t)f);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.40f * br, h0);
        gk_col(pal, pi + 2200, 0.05f, 0.40f * br, h1);
        gk_col(pal, pi + 300, 0.55f, 0.65f * br, c0);
        gk_col(pal, pi + 2500, 0.45f, 0.65f * br, c1);
        for (int k = 0; k < 2; k++) {
            float wgt = k == 0 ? wa : wb;
            if (wgt <= 0.001f) continue;
            const gk_bolt *b = &b519[f][k == 0 ? j : j ^ 1];
            bx_draw_grad(&g519, b, cx, cy, ang, len, 1.0f, 2.0f, wgt, h0, h1, 0.0f, 1.6f * sc, 5.5f * sc, 0.5f);
            bx_draw_grad(&g519, b, cx, cy, ang, len, 1.0f, 2.0f, wgt, c0, c1, 0.0f, 0.7f * sc, 1.8f * sc, 0.25f);
        }
        float tc[3];
        gk_col(pal, pi + 2500, 0.3f, 0.6f * br, tc);
        gk_dot(&g519, ex, ey, tc, 3.0f * sc, 14.0f * sc, 0.6f);
    }
    float ec[3], eh[3];
    gk_col(pal, base + 1000, 0.5f, 1.0f, ec);
    gk_col(pal, base + 1600, 0.1f, 0.5f, eh);
    gk_dot(&g519, cx, cy, eh, 8.0f * sc, 30.0f * sc, 0.6f);
    gk_dot(&g519, cx, cy, ec, 4.0f * sc, 12.0f * sc, 0.6f);
    gk_present(&g519, fb, w, h);
}
