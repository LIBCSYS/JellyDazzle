/* 510 Lissajous Bolt — a discharge traces a whole Lissajous figure (3:2,
 * 4:3, 5:4 by turns), the tip travelling the closed curve over ~320 frames
 * with stable per-point jag and twig sparks, so a knotted luminous figure
 * fills the frame; it holds, the hue sliding along its length, then fades
 * while the next figure (new ratio, new hue) begins to be drawn.  Figure
 * overlay across most of the frame.  Repaint. */
#include "_trace509.h"

#define P510 470

static gk g510;

typedef struct { float cx, cy, ax, ay, ph, fa, fb; } lis510;

static void f510(float u, void *vc, float *x, float *y)
{
    lis510 *c = (lis510 *)vc;
    float t = u * GK_TAU;
    *x = c->cx + c->ax * sinf(c->fa * t + c->ph);
    *y = c->cy + c->ay * sinf(c->fb * t);
}

void pattern_510(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g510, w, h);
    gk_clear(&g510);
    float cw = (float)g510.cw, ch = (float)g510.ch, sc = g510.sc, t = (float)frame;
    int base = (int)(t * 1.2f) + (int)(seed & 8191u);
    static const float ratios[4][2] = { { 3, 2 }, { 4, 3 }, { 5, 4 }, { 3, 4 } };
    for (int k = 0; k < 2; k++) {
        int idx = frame / P510 - k;
        if (idx < 0) continue;
        float age = (float)(frame - idx * P510);
        float env = gk_env(age, 10.0f, 380.0f, 130.0f);
        if (env <= 0.0f) continue;
        uint32_t hs = (uint32_t)idx * 6547u + seed * 3u;
        lis510 c;
        c.cx = cw * 0.5f; c.cy = ch * 0.5f;
        c.ax = cw * (0.36f + 0.06f * gk_hash(hs + 1u));
        c.ay = ch * (0.34f + 0.08f * gk_hash(hs + 2u));
        int ri = (int)(gk_hash(hs + 3u) * 4.0f) & 3;
        c.fa = ratios[ri][0]; c.fb = ratios[ri][1];
        c.ph = GK_TAU * 0.25f + 0.02f * sinf(t * 0.005f + (float)idx);   /* very slow phase breathing */
        tr_spec s;
        s.f = f510; s.ctx = &c; s.n = 420;
        s.prog = age / 320.0f; s.env = env;
        s.pi0 = base + (int)(gk_hash(hs + 5u) * 7000.0f); s.hspan = 5000;
        s.hs = hs; s.jit = 5.0f * sc; s.sc = sc; s.wt = 0.55f; s.thick = 1.1f;
        s.mirx = 0; s.mx = 0.0f; s.twigs = 1;
        tr_draw(&g510, pal, &s);
    }
    gk_present(&g510, fb, w, h);
}
