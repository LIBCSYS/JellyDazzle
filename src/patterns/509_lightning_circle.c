/* 509 Lightning Circle — a bolt that traces a circle: the leader tip runs
 * round a large ring over ~200 frames, its jag stable per point, throwing
 * small twig sparks in and out as it passes; the ring holds, breathes in
 * radius, and fades while the next (other direction, other radius, other
 * hue) starts.  Hue slides round the ring and drifts with time.  Figure
 * overlay, centre-weighted.  Repaint. */
#include "_trace509.h"

#define P509 330

static gk g509;

typedef struct { float cx, cy, r, a0, hand, wob; uint32_t k; } circ509;

static void f509(float u, void *vc, float *x, float *y)
{
    circ509 *c = (circ509 *)vc;
    float a = c->a0 + c->hand * u * GK_TAU;
    float r = c->r * (1.0f + 0.05f * gk_noise1(u * 6.0f + 3.0f, c->k) - 0.025f) * c->wob;
    *x = c->cx + cosf(a) * r; *y = c->cy + sinf(a) * r;
}

void pattern_509(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g509, w, h);
    gk_clear(&g509);
    float cw = (float)g509.cw, ch = (float)g509.ch, sc = g509.sc, t = (float)frame;
    float rmax = (cw < ch ? cw : ch) * 0.42f;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    for (int k = 0; k < 2; k++) {
        int idx = frame / P509 - k;
        if (idx < 0) continue;
        float age = (float)(frame - idx * P509);
        float env = gk_env(age, 10.0f, 260.0f, 130.0f);
        if (env <= 0.0f) continue;
        uint32_t hs = (uint32_t)idx * 7919u + seed;
        circ509 c;
        c.cx = cw * 0.5f + cw * 0.06f * (gk_hash(hs + 1u) - 0.5f);
        c.cy = ch * 0.5f + ch * 0.06f * (gk_hash(hs + 2u) - 0.5f);
        c.r = rmax * (0.72f + 0.28f * gk_hash(hs + 3u));
        c.a0 = gk_hash(hs + 4u) * GK_TAU;
        c.hand = (idx & 1) ? 1.0f : -1.0f;
        c.wob = 1.0f + 0.02f * sinf(t * 0.011f + (float)idx);
        c.k = hs;
        tr_spec s;
        s.f = f509; s.ctx = &c; s.n = 240;
        s.prog = age / 200.0f; s.env = env;
        s.pi0 = base + (int)(gk_hash(hs + 5u) * 6000.0f); s.hspan = 4000;
        s.hs = hs; s.jit = 7.0f * sc; s.sc = sc; s.wt = 0.55f; s.thick = 1.25f;
        s.mirx = 0; s.mx = 0.0f; s.twigs = 1;
        tr_draw(&g509, pal, &s);
        /* soft hub */
        float hub[3];
        gk_col(pal, s.pi0 + 2000, 0.3f, 0.5f * env, hub);
        gk_dot(&g509, c.cx, c.cy, hub, 2.5f * sc, 12.0f * sc, 0.5f);
    }
    gk_present(&g509, fb, w, h);
}
