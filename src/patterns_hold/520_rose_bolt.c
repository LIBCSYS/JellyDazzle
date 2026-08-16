/* 520 Rose Bolt — a discharge traces a rose curve (r = R cos k·theta, k of
 * 3, 4, 5 or 7 by turns) petal after petal over ~300 frames, jagged and
 * twig-sparked, so a flower of lightning blooms from the centre; it holds,
 * turning almost imperceptibly, hue sliding petal to petal, then fades
 * while the next rose (new k, new hue) begins.  Figure overlay, centre-
 * weighted.  Repaint. */
#include "_trace509.h"

#define P520 460

static gk g520;

typedef struct { float cx, cy, R, rot; int k; float span; } rose520;

static void f520(float u, void *vc, float *x, float *y)
{
    rose520 *c = (rose520 *)vc;
    float th = u * c->span;
    float r = c->R * cosf((float)c->k * th);
    float a = th + c->rot;
    *x = c->cx + cosf(a) * r; *y = c->cy + sinf(a) * r;
}

void pattern_520(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g520, w, h);
    gk_clear(&g520);
    float cw = (float)g520.cw, ch = (float)g520.ch, sc = g520.sc, t = (float)frame;
    int base = (int)(t * 1.2f) + (int)(seed & 8191u);
    static const int ks[4] = { 3, 4, 5, 7 };
    for (int k = 0; k < 2; k++) {
        int idx = frame / P520 - k;
        if (idx < 0) continue;
        float age = (float)(frame - idx * P520);
        float env = gk_env(age, 10.0f, 370.0f, 130.0f);
        if (env <= 0.0f) continue;
        uint32_t hs = (uint32_t)idx * 4649u + seed * 5u;
        rose520 c;
        c.cx = cw * 0.5f; c.cy = ch * 0.5f;
        c.R = (cw < ch ? cw : ch) * (0.38f + 0.08f * gk_hash(hs + 1u));
        c.k = ks[(int)(gk_hash(hs + 2u) * 4.0f) & 3];
        c.span = (c.k & 1) ? GK_TAU * 0.5f : GK_TAU;
        c.rot = gk_hash(hs + 3u) * GK_TAU + t * 0.0006f * ((idx & 1) ? 1.0f : -1.0f);
        tr_spec s;
        s.f = f520; s.ctx = &c; s.n = 96 * c.k;
        s.prog = age / 300.0f; s.env = env;
        s.pi0 = base + (int)(gk_hash(hs + 5u) * 7000.0f); s.hspan = 5000;
        s.hs = hs; s.jit = 3.5f * sc; s.sc = sc; s.wt = 0.55f; s.thick = 1.1f;
        s.mirx = 0; s.mx = 0.0f; s.twigs = 1;
        tr_draw(&g520, pal, &s);
        float hub[3];
        gk_col(pal, s.pi0 + 2500, 0.4f, 0.7f * env, hub);
        gk_dot(&g520, c.cx, c.cy, hub, 3.0f * sc, 14.0f * sc, 0.5f);
    }
    gk_present(&g520, fb, w, h);
}
