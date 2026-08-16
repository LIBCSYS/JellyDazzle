/* _trace509.h — extras on top of _glow469.h for patterns 509..540:
 *   bx_draw_grad : draw a gk_bolt with the colour lerped along its arc-time
 *                  (c0 at the root, c1 at the tips) so hue varies ALONG a
 *                  bolt, plus an optional taper toward the tips;
 *   tr_draw      : trace a parametric curve as a jagged luminous line that
 *                  reveals tip-first (prog 0..1), hue sliding along it,
 *                  with little twig sparks hung off it and a hot tip dot.
 * Everything static, per translation unit. */
#ifndef JD_TRACE509_H
#define JD_TRACE509_H
#include "_glow469.h"

static void bx_draw_grad(gk *g, const gk_bolt *b, float ox, float oy, float rot, float scl, float mir,
                         float prog, float amp, const float *c0, const float *c1, float taper,
                         float cr, float gr, float gi)
{
    int i;
    float col[3];
    float ca = cosf(rot) * scl, sa = sinf(rot) * scl;
    for (i = 0; i < b->n; i++) {
        const gk_bseg *s = &b->s[i];
        if (s->t0 >= prog) continue;
        float x1 = s->x1, y1 = s->y1, t1 = s->t1;
        if (s->t1 > prog) {
            float f = (prog - s->t0) / (s->t1 - s->t0);
            x1 = s->x0 + (s->x1 - s->x0) * f;
            y1 = s->y0 + (s->y1 - s->y0) * f;
            t1 = prog;
        }
        float tm = 0.5f * (s->t0 + t1);
        if (tm > 1.0f) tm = 1.0f;
        float wa = amp * (0.35f + 0.65f * s->wgt) * (1.0f - taper * tm);
        if (wa <= 0.0f) continue;
        col[0] = (c0[0] + (c1[0] - c0[0]) * tm) * wa;
        col[1] = (c0[1] + (c1[1] - c0[1]) * tm) * wa;
        col[2] = (c0[2] + (c1[2] - c0[2]) * tm) * wa;
        float ws = 0.5f + 0.5f * s->wgt;
        float sy0 = s->y0 * mir; y1 *= mir;
        gk_seg(g, ox + s->x0 * ca - sy0 * sa, oy + s->x0 * sa + sy0 * ca,
                  ox + x1 * ca - y1 * sa,     oy + x1 * sa + y1 * ca,
               col, cr * ws, gr * ws, gi);
    }
}

typedef void (*tr_fn)(float u, void *ctx, float *x, float *y);

typedef struct {
    tr_fn f; void *ctx;
    int n;            /* sample points along the curve            */
    float prog;       /* revealed fraction 0..1                    */
    float env;        /* amplitude envelope 0..1                   */
    int pi0, hspan;   /* palette index at u=0, and span to u=1     */
    uint32_t hs;      /* hash salt for the stable jitter           */
    float jit;        /* jitter amplitude, canvas px               */
    float sc;         /* canvas scale                              */
    float wt;         /* core whitening 0..1                       */
    float thick;      /* radius multiplier                         */
    int mirx; float mx;   /* also draw mirrored about x = mx       */
    int twigs;        /* 1 = hang twig sparks off the trace        */
} tr_spec;

static void tr_seg2(gk *g, float x0, float y0, float x1, float y1, const float *hc, const float *c,
                    float sc, float th, float fade)
{
    float c1[3] = { hc[0] * fade, hc[1] * fade, hc[2] * fade };
    float c2[3] = { c[0] * fade, c[1] * fade, c[2] * fade };
    gk_seg(g, x0, y0, x1, y1, c1, 1.8f * sc * th, 6.0f * sc * th, 0.5f);
    gk_seg(g, x0, y0, x1, y1, c2, 0.8f * sc * th, 2.0f * sc * th, 0.25f);
}

static void tr_draw(gk *g, const uint32_t *pal, const tr_spec *s)
{
    int n = s->n, i;
    float env = s->env;
    if (env <= 0.0f || s->prog <= 0.0f) return;
    float lx = 0.0f, ly = 0.0f, lxm = 0.0f;
    float tipx = 0.0f, tipy = 0.0f;
    for (i = 0; i < n; i++) {
        float u = (float)i / (float)(n - 1);
        if (u > s->prog + 1.0f / (float)n) break;
        float x, y, xa, ya, xb, yb;
        s->f(u, s->ctx, &x, &y);
        s->f(u - 0.004f, s->ctx, &xa, &ya);
        s->f(u + 0.004f, s->ctx, &xb, &yb);
        float tx = xb - xa, ty = yb - ya, tl = sqrtf(tx * tx + ty * ty);
        if (tl < 1e-5f) { tx = 1.0f; ty = 0.0f; } else { tx /= tl; ty /= tl; }
        float nx = -ty, ny = tx;
        float j = (gk_hash((uint32_t)i * 29u + s->hs) - 0.5f) * 2.0f * s->jit;
        x += nx * j; y += ny * j;
        /* partial last point: slide it back toward the previous one */
        if (u > s->prog && i > 0) {
            float f = 1.0f - (u - s->prog) * (float)(n - 1);
            if (f < 0.0f) f = 0.0f;
            x = lx + (x - lx) * f; y = ly + (y - ly) * f;
        }
        int pi = s->pi0 + (int)(u * (float)s->hspan);
        float hc[3], c[3];
        gk_col(pal, pi + 700, 0.05f, 0.45f * env, hc);
        gk_col(pal, pi, s->wt, 0.7f * env, c);
        if (i > 0) {
            tr_seg2(g, lx, ly, x, y, hc, c, s->sc, s->thick, 1.0f);
            if (s->mirx) tr_seg2(g, 2.0f * s->mx - lx, ly, 2.0f * s->mx - x, y, hc, c, s->sc, s->thick, 1.0f);
        }
        /* twig sparks: about one point in eight */
        if (s->twigs && i > 0 && u <= s->prog && gk_hash((uint32_t)i * 53u + s->hs + 77u) < 0.13f) {
            float tp = (s->prog - u) * (float)n / 8.0f;      /* grows over 8 points of prog */
            if (tp > 1.0f) tp = 1.0f;
            if (tp > 0.0f) {
                float side = gk_hash((uint32_t)i * 71u + s->hs) < 0.5f ? -1.0f : 1.0f;
                float ang = atan2f(ny * side, nx * side) + (gk_hash((uint32_t)i * 91u + s->hs) - 0.5f) * 0.8f;
                float len = (10.0f + 24.0f * gk_hash((uint32_t)i * 17u + s->hs)) * s->sc * tp;
                float tc[3], thc[3];
                gk_col(pal, pi + 1400, 0.3f, 0.5f * env, tc);
                gk_col(pal, pi + 2100, 0.05f, 0.25f * env, thc);
                float px = x, py = y, k;
                for (k = 1.0f; k <= 4.0f; k += 1.0f) {
                    float a2 = ang + (gk_hash((uint32_t)i * 13u + (uint32_t)k * 101u + s->hs) - 0.5f) * 0.9f;
                    float qx = px + cosf(a2) * len * 0.25f, qy = py + sinf(a2) * len * 0.25f;
                    float fd = 1.0f - 0.18f * k;
                    tr_seg2(g, px, py, qx, qy, thc, tc, s->sc, s->thick * 0.6f, fd);
                    if (s->mirx) tr_seg2(g, 2.0f * s->mx - px, py, 2.0f * s->mx - qx, qy, thc, tc, s->sc, s->thick * 0.6f, fd);
                    px = qx; py = qy;
                }
            }
        }
        lx = x; ly = y; lxm = 2.0f * s->mx - x;
        tipx = x; tipy = y;
    }
    if (s->prog < 1.0f) {
        float tc[3];
        int pi = s->pi0 + (int)(s->prog * (float)s->hspan);
        gk_col(pal, pi, 0.75f, 1.0f * env, tc);
        gk_dot(g, tipx, tipy, tc, 1.6f * s->sc * s->thick, 6.0f * s->sc * s->thick, 0.6f);
        if (s->mirx) gk_dot(g, lxm, tipy, tc, 1.6f * s->sc * s->thick, 6.0f * s->sc * s->thick, 0.6f);
    }
}
#endif
