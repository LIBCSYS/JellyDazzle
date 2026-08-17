/* 521 Star Bolt — a discharge traces a star polygon in one stroke (a
 * pentagram {5/2}, a heptagram {7/3} or an octagram {8/3} by turns), edge
 * after edge over ~260 frames, each vertex flaring as the leader passes it
 * and the whole star turning slowly; hue slides edge to edge and drifts
 * with time; holds, fades while the next star begins.  Figure overlay,
 * centre-weighted.  Repaint. */
#include "_trace509.h"

#define P521 420

static gk g521;

typedef struct { float cx, cy, R, rot; int n, step; } star521;

static void vert521(const star521 *c, int i, float *x, float *y)
{
    int v = (i * c->step) % c->n;
    float a = c->rot + GK_TAU * (float)v / (float)c->n - GK_TAU * 0.25f;
    *x = c->cx + cosf(a) * c->R; *y = c->cy + sinf(a) * c->R;
}
static void f521(float u, void *vc, float *x, float *y)
{
    star521 *c = (star521 *)vc;
    if (u < 0.0f) u = 0.0f; if (u > 1.0f) u = 1.0f;
    float e = u * (float)c->n; int i = (int)e; if (i >= c->n) i = c->n - 1;
    float f = e - (float)i;
    float xa, ya, xb, yb;
    vert521(c, i, &xa, &ya); vert521(c, i + 1, &xb, &yb);
    *x = xa + (xb - xa) * f; *y = ya + (yb - ya) * f;
}

void pattern_521(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g521, w, h);
    gk_clear(&g521);
    float cw = (float)g521.cw, ch = (float)g521.ch, sc = g521.sc, t = (float)frame;
    int base = (int)(t * 1.2f) + (int)(seed & 8191u);
    static const int ns[3] = { 5, 7, 8 }, st[3] = { 2, 3, 3 };
    for (int k = 0; k < 2; k++) {
        int idx = frame / P521 - k;
        if (idx < 0) continue;
        float age = (float)(frame - idx * P521);
        float env = gk_env(age, 10.0f, 330.0f, 130.0f);
        if (env <= 0.0f) continue;
        uint32_t hs = (uint32_t)idx * 5197u + seed * 7u;
        star521 c;
        c.cx = cw * 0.5f; c.cy = ch * 0.5f;
        c.R = (cw < ch ? cw : ch) * (0.36f + 0.1f * gk_hash(hs + 1u));
        int which = (int)(gk_hash(hs + 2u) * 3.0f) % 3;
        c.n = ns[which]; c.step = st[which];
        c.rot = gk_hash(hs + 3u) * GK_TAU + t * 0.0007f * ((idx & 1) ? 1.0f : -1.0f);
        tr_spec s;
        s.f = f521; s.ctx = &c; s.n = 60 * c.n;
        s.prog = age / 260.0f; s.env = env;
        s.pi0 = base + (int)(gk_hash(hs + 5u) * 7000.0f); s.hspan = 4500;
        s.hs = hs; s.jit = 6.0f * sc; s.sc = sc; s.wt = 0.55f; s.thick = 1.15f;
        s.mirx = 0; s.mx = 0.0f; s.twigs = 1;
        tr_draw(&g521, pal, &s);
        /* vertices flare as the leader passes, then settle to a soft glow */
        for (int i = 0; i <= c.n; i++) {
            float ui = (float)i / (float)c.n;
            float since = (s.prog - ui) * 260.0f;
            if (since < 0.0f) continue;
            float fl = 0.35f + 0.65f * expf(-since / 40.0f);
            float vx, vy, vc[3], vh[3];
            vert521(&c, i, &vx, &vy);
            int pi = s.pi0 + (int)(ui * (float)s.hspan);
            gk_col(pal, pi + 500, 0.5f, 0.8f * env * fl, vc);
            gk_col(pal, pi + 1200, 0.1f, 0.35f * env * fl, vh);
            gk_dot(&g521, vx, vy, vh, 5.0f * sc, 20.0f * sc, 0.6f);
            gk_dot(&g521, vx, vy, vc, 2.0f * sc, 7.0f * sc, 0.6f);
        }
    }
    gk_present(&g521, fb, w, h);
}
