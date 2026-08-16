/* 526 Lightning Heart — two leaders leave the bottom point of a heart
 * curve together and climb its two mirrored sides over ~240 frames, jagged
 * and twig-sparked, meeting at the notch on top with a soft bloom; the
 * heart then holds and beats (a slow 1-2% swell), hue sliding up the
 * sides and drifting with time, before fading while the next heart (new
 * size, new hue) begins from the point.  Figure overlay, centre-weighted.
 * Repaint. */
#include "_trace509.h"

#define P526 400

static gk g526;

typedef struct { float cx, cy, s; } heart526;

/* right half only, u=0 at the bottom point (t = pi) rising to the notch (t = 0) */
static void f526(float u, void *vc, float *x, float *y)
{
    heart526 *c = (heart526 *)vc;
    if (u < 0.0f) u = 0.0f; if (u > 1.0f) u = 1.0f;
    float t = GK_TAU * 0.5f * (1.0f - u);
    float hx = 16.0f * sinf(t) * sinf(t) * sinf(t);
    float hy = 13.0f * cosf(t) - 5.0f * cosf(2.0f * t) - 2.0f * cosf(3.0f * t) - cosf(4.0f * t);
    *x = c->cx + hx * c->s; *y = c->cy - hy * c->s;
}

void pattern_526(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g526, w, h);
    gk_clear(&g526);
    float cw = (float)g526.cw, ch = (float)g526.ch, sc = g526.sc, t = (float)frame;
    int base = (int)(t * 1.2f) + (int)(seed & 8191u);
    for (int k = 0; k < 2; k++) {
        int idx = frame / P526 - k;
        if (idx < 0) continue;
        float age = (float)(frame - idx * P526);
        float env = gk_env(age, 10.0f, 300.0f, 130.0f);
        if (env <= 0.0f) continue;
        uint32_t hs = (uint32_t)idx * 3733u + seed * 9u;
        heart526 c;
        float beat = 1.0f + 0.018f * (0.5f + 0.5f * sinf(t * 0.03f)) * gk_smooth((age - 240.0f) / 40.0f);
        c.s = (cw < ch ? cw : ch) * (0.024f + 0.005f * gk_hash(hs + 1u)) * beat;
        c.cx = cw * 0.5f; c.cy = ch * 0.5f - 6.0f * c.s;
        tr_spec s;
        s.f = f526; s.ctx = &c; s.n = 200;
        s.prog = age / 240.0f; s.env = env;
        s.pi0 = base + (int)(gk_hash(hs + 5u) * 7000.0f); s.hspan = 4000;
        s.hs = hs; s.jit = 4.5f * sc; s.sc = sc; s.wt = 0.55f; s.thick = 1.2f;
        s.mirx = 1; s.mx = c.cx; s.twigs = 1;
        tr_draw(&g526, pal, &s);
        /* notch bloom on meeting, then a soft resident glow */
        float bl = gk_env(age - 235.0f, 20.0f, 30.0f, 200.0f) * 0.8f + 0.25f * gk_smooth((age - 240.0f) / 60.0f);
        if (bl > 0.0f) {
            float nx, ny, bc[3], bh[3];
            f526(1.0f, &c, &nx, &ny);
            gk_col(pal, s.pi0 + 4000, 0.5f, 0.9f * bl * env, bc);
            gk_col(pal, s.pi0 + 4600, 0.1f, 0.4f * bl * env, bh);
            gk_dot(&g526, nx, ny, bh, 8.0f * sc, 30.0f * sc, 0.6f);
            gk_dot(&g526, nx, ny, bc, 2.5f * sc, 9.0f * sc, 0.6f);
        }
        /* bottom point glow */
        float px, py, pc[3];
        f526(0.0f, &c, &px, &py);
        gk_col(pal, s.pi0 + 300, 0.4f, 0.6f * env, pc);
        gk_dot(&g526, px, py, pc, 2.5f * sc, 12.0f * sc, 0.6f);
    }
    gk_present(&g526, fb, w, h);
}
