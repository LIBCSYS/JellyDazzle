/* _glow469.h — shared drawing kit for patterns 469..601 (lightning family and
 * the figure/spark overlays).  Everything is static: each translation unit
 * gets its own private copy, nothing but pattern_NNN escapes.
 *
 * Model: a float RGB canvas of at most ~700 px across (the framebuffer
 * halved until it fits), drawn into with additive Gaussian "capsules"
 * (segments) and dots — a bright core plus a wide soft halo — then
 * tone-mapped 1-exp(-v) and bilinear-upscaled through _upsample.h.
 * Black stays black, so anything not drawn is transparent to the layers
 * beneath under MAX/screen blending.
 *
 * Also here: a xorshift RNG, smooth value noise for slow wander, palette
 * sampling helpers, and a midpoint-displacement bolt generator whose
 * segments carry an arc-time so a bolt can be revealed tip-first over
 * many frames instead of popping. */
#ifndef JD_GLOW469_H
#define JD_GLOW469_H
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GK_TAU 6.28318530717958647692f

typedef struct {
    int cw, ch, cap;      /* canvas dims and allocated pixel capacity */
    float *acc;           /* cw*ch*3 linear light                    */
    uint8_t *img;         /* cw*ch*3 tone-mapped                     */
    jd_up up;
    uint32_t rs;          /* rng state                               */
    float sc;             /* cw / 640 — scale for radii              */
} gk;

static float   gk_etab[1040];   /* exp(-x), x = i/128, 0..8.1  */
static uint8_t gk_ttab[4100];   /* 255*(1-exp(-v)), v = i/512  */
static int     gk_tabs_ok;

static void gk_tabs(void)
{
    int i;
    if (gk_tabs_ok) return;
    for (i = 0; i < 1040; i++) gk_etab[i] = expf(-(float)i / 128.0f);
    for (i = 0; i < 4100; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i / 512.0f));
        gk_ttab[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    gk_tabs_ok = 1;
}

/* ---- rng ---------------------------------------------------------- */
static inline uint32_t gk_ru(gk *g)
{
    uint32_t x = g->rs;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    g->rs = x ? x : 0x9E3779B9u;
    return g->rs;
}
static inline float gk_rf(gk *g) { return (float)(gk_ru(g) >> 8) * (1.0f / 16777216.0f); }
static inline float gk_rs(gk *g) { return gk_rf(g) * 2.0f - 1.0f; }
static inline void  gk_seed(gk *g, uint32_t s) { g->rs = s ? s * 2654435761u : 0x9E3779B9u; gk_ru(g); gk_ru(g); }

/* integer hash -> 0..1 */
static inline float gk_hash(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return (float)(x >> 8) * (1.0f / 16777216.0f);
}
/* smooth 1-D value noise, 0..1, period-free; k selects a stream */
static inline float gk_noise1(float t, uint32_t k)
{
    float f = floorf(t), u = t - f;
    uint32_t i = (uint32_t)(int)f;
    u = u * u * (3.0f - 2.0f * u);
    float a = gk_hash(i * 0x9E37u + k * 0x85EBCA6Bu);
    float b = gk_hash((i + 1u) * 0x9E37u + k * 0x85EBCA6Bu);
    return a + (b - a) * u;
}
/* 2-D value noise, 0..1 */
static inline float gk_noise2(float x, float y, uint32_t k)
{
    float fx = floorf(x), fy = floorf(y);
    float ux = x - fx, uy = y - fy;
    uint32_t ix = (uint32_t)(int)fx, iy = (uint32_t)(int)fy;
    ux = ux * ux * (3.0f - 2.0f * ux); uy = uy * uy * (3.0f - 2.0f * uy);
    uint32_t h00 = ix * 0x9E37u + iy * 0x68E31DA4u + k * 0x85EBCA6Bu;
    float a = gk_hash(h00), b = gk_hash(h00 + 0x9E37u);
    float c = gk_hash(h00 + 0x68E31DA4u), d = gk_hash(h00 + 0x9E37u + 0x68E31DA4u);
    float t = a + (b - a) * ux, s = c + (d - c) * ux;
    return t + (s - t) * uy;
}

/* ---- canvas ------------------------------------------------------- */
static void gk_setup(gk *g, int w, int h)
{
    int cw = w, ch = h;
    gk_tabs();
    while (cw > 700 || ch > 700) { cw = (cw + 1) >> 1; ch = (ch + 1) >> 1; }
    if (cw < 8) cw = 8;
    if (ch < 8) ch = 8;
    if (cw != g->cw || ch != g->ch || !g->acc) {
        int n = cw * ch;
        if (n > g->cap || !g->acc) {
            free(g->acc); free(g->img);
            g->acc = (float *)calloc((size_t)n * 3, sizeof(float));
            g->img = (uint8_t *)calloc((size_t)n * 3, 1);
            g->cap = n;
        } else {
            memset(g->acc, 0, sizeof(float) * (size_t)n * 3);
        }
        g->cw = cw; g->ch = ch;
        g->sc = (float)cw / 640.0f;
    }
}
static inline void gk_clear(gk *g)
{
    memset(g->acc, 0, sizeof(float) * (size_t)(g->cw * g->ch) * 3);
}
static inline void gk_decay(gk *g, float k)
{
    int i, n = g->cw * g->ch * 3;
    float *a = g->acc;
    for (i = 0; i < n; i++) a[i] *= k;
}
/* decay with a floor snap so trails do actually reach black */
static inline void gk_decay_snap(gk *g, float k)
{
    int i, n = g->cw * g->ch * 3;
    float *a = g->acc;
    for (i = 0; i < n; i++) { float v = a[i] * k; a[i] = v < 0.002f ? 0.0f : v; }
}

/* tone-map and blit into the framebuffer (fully overwrites fb) */
static void gk_present(gk *g, uint32_t *fb, int w, int h)
{
    int i, n = g->cw * g->ch * 3;
    const float *a = g->acc;
    uint8_t *o = g->img;
    for (i = 0; i < n; i++) {
        float v = a[i];
        int t = (int)(v * 512.0f);
        o[i] = gk_ttab[t < 0 ? 0 : t > 4096 ? 4096 : t];
    }
    jd_up_blit(&g->up, fb, w, h, g->img, g->cw, g->ch);
}

/* ---- colour --------------------------------------------------------- */
/* palette sample normalised so max channel = 1, mixed toward white by wt,
 * scaled by amp.  Colour still comes only from pal; the whitening is what
 * a hot core does to any hue. */
static inline void gk_col(const uint32_t *pal, int idx, float wt, float amp, float *out)
{
    uint32_t p = pal[idx & JD_PAL_MASK];
    float r = (float)((p >> 16) & 255), g = (float)((p >> 8) & 255), b = (float)(p & 255);
    float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    float n = 1.0f / mx, k = 1.0f - wt;
    out[0] = (wt + k * r * n) * amp;
    out[1] = (wt + k * g * n) * amp;
    out[2] = (wt + k * b * n) * amp;
}
/* raw palette sample 0..1 per channel, scaled by amp (keeps palette darkness) */
static inline void gk_colraw(const uint32_t *pal, int idx, float amp, float *out)
{
    uint32_t p = pal[idx & JD_PAL_MASK];
    out[0] = (float)((p >> 16) & 255) * (amp / 255.0f);
    out[1] = (float)((p >> 8) & 255) * (amp / 255.0f);
    out[2] = (float)(p & 255) * (amp / 255.0f);
}

/* ---- primitives ------------------------------------------------------ */
/* Gaussian capsule: intensity(d) = exp(-(d/cr)^2) + gi*exp(-(d/gr)^2),
 * cut at d = 2*gr.  Coordinates in canvas pixels. */
static void gk_seg(gk *g, float x0, float y0, float x1, float y1,
                   const float *c, float cr, float gr, float gi)
{
    float R = gr * 2.0f;
    int cw = g->cw, ch = g->ch;
    float mnx = x0 < x1 ? x0 : x1, mxx = x0 < x1 ? x1 : x0;
    float mny = y0 < y1 ? y0 : y1, mxy = y0 < y1 ? y1 : y0;
    int xa = (int)floorf(mnx - R), xb = (int)ceilf(mxx + R);
    int ya = (int)floorf(mny - R), yb = (int)ceilf(mxy + R);
    if (xa < 0) xa = 0; if (ya < 0) ya = 0;
    if (xb >= cw) xb = cw - 1; if (yb >= ch) yb = ch - 1;
    if (xa > xb || ya > yb) return;
    float dx = x1 - x0, dy = y1 - y0;
    float l2 = dx * dx + dy * dy;
    float il2 = l2 > 1e-6f ? 1.0f / l2 : 0.0f;
    float icr = 128.0f / (cr * cr), igr = 128.0f / (gr * gr);
    float R2 = R * R;
    int x, y;
    for (y = ya; y <= yb; y++) {
        float py = (float)y + 0.5f - y0;
        float *row = g->acc + ((size_t)y * (size_t)cw) * 3;
        for (x = xa; x <= xb; x++) {
            float px = (float)x + 0.5f - x0;
            float t = (px * dx + py * dy) * il2;
            if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
            float ex = px - t * dx, ey = py - t * dy;
            float d2 = ex * ex + ey * ey;
            if (d2 > R2) continue;
            int kc = (int)(d2 * icr), kg = (int)(d2 * igr);
            float v = (kc < 1024 ? gk_etab[kc] : 0.0f) + gi * gk_etab[kg < 1024 ? kg : 1024];
            row[x * 3 + 0] += c[0] * v;
            row[x * 3 + 1] += c[1] * v;
            row[x * 3 + 2] += c[2] * v;
        }
    }
}
static void gk_dot(gk *g, float x0, float y0, const float *c, float cr, float gr, float gi)
{
    gk_seg(g, x0, y0, x0, y0, c, cr, gr, gi);
}
/* flat-ish soft disc: (1 - (d/r)^2)^p style, p fixed at 2, for bodies */
static void gk_disc(gk *g, float x0, float y0, float r, const float *c)
{
    int cw = g->cw, ch = g->ch;
    int xa = (int)floorf(x0 - r), xb = (int)ceilf(x0 + r);
    int ya = (int)floorf(y0 - r), yb = (int)ceilf(y0 + r);
    if (xa < 0) xa = 0; if (ya < 0) ya = 0;
    if (xb >= cw) xb = cw - 1; if (yb >= ch) yb = ch - 1;
    float ir2 = 1.0f / (r * r);
    int x, y;
    for (y = ya; y <= yb; y++) {
        float py = (float)y + 0.5f - y0;
        float *row = g->acc + ((size_t)y * (size_t)cw) * 3;
        for (x = xa; x <= xb; x++) {
            float px = (float)x + 0.5f - x0;
            float q = 1.0f - (px * px + py * py) * ir2;
            if (q <= 0.0f) continue;
            float v = q * q;
            row[x * 3 + 0] += c[0] * v;
            row[x * 3 + 1] += c[1] * v;
            row[x * 3 + 2] += c[2] * v;
        }
    }
}
/* soft ring: gaussian around radius r with width wd */
static void gk_ring(gk *g, float x0, float y0, float r, float wd, const float *c)
{
    int cw = g->cw, ch = g->ch;
    float R = r + wd * 2.5f;
    int xa = (int)floorf(x0 - R), xb = (int)ceilf(x0 + R);
    int ya = (int)floorf(y0 - R), yb = (int)ceilf(y0 + R);
    if (xa < 0) xa = 0; if (ya < 0) ya = 0;
    if (xb >= cw) xb = cw - 1; if (yb >= ch) yb = ch - 1;
    float iw = 128.0f / (wd * wd);
    float rin = r - wd * 2.5f; if (rin < 0.0f) rin = 0.0f;
    float rin2 = rin * rin, R2 = R * R;
    int x, y;
    for (y = ya; y <= yb; y++) {
        float py = (float)y + 0.5f - y0;
        float *row = g->acc + ((size_t)y * (size_t)cw) * 3;
        for (x = xa; x <= xb; x++) {
            float px = (float)x + 0.5f - x0;
            float d2 = px * px + py * py;
            if (d2 > R2 || d2 < rin2) continue;
            float d = sqrtf(d2) - r;
            int k = (int)(d * d * iw);
            float v = gk_etab[k < 1024 ? k : 1024];
            row[x * 3 + 0] += c[0] * v;
            row[x * 3 + 1] += c[1] * v;
            row[x * 3 + 2] += c[2] * v;
        }
    }
}
/* polygon outline through n points (closed), glow segments */
static void gk_poly(gk *g, const float *xs, const float *ys, int n,
                    const float *c, float cr, float gr, float gi)
{
    int i;
    for (i = 0; i < n; i++) {
        int j = (i + 1) % n;
        gk_seg(g, xs[i], ys[i], xs[j], ys[j], c, cr, gr, gi);
    }
}

/* ---- bolts -------------------------------------------------------------- */
#define GK_MAXSEG 1024
typedef struct { float x0, y0, x1, y1, wgt, t0, t1; } gk_bseg;
typedef struct { gk_bseg s[GK_MAXSEG]; int n; } gk_bolt;

/* one channel from A to B by recursive midpoint displacement; segments are
 * appended with arc-times spanning ta..tb and weight wgt.  Points land in
 * pts (2^depth+1 of them) so branches can be hung off them afterwards. */
#define GK_MAXPTS 257
static void gk_bolt_channel(gk *g, gk_bolt *b, float x0, float y0, float x1, float y1,
                            float jag, int depth, float ta, float tb, float wgt,
                            float *px, float *py, int *np)
{
    if (depth <= 0) {
        if (b->n < GK_MAXSEG) {
            gk_bseg *s = &b->s[b->n++];
            s->x0 = x0; s->y0 = y0; s->x1 = x1; s->y1 = y1;
            s->wgt = wgt; s->t0 = ta; s->t1 = tb;
        }
        if (*np < GK_MAXPTS) { px[*np] = x1; py[*np] = y1; (*np)++; }
        return;
    }
    float mx = 0.5f * (x0 + x1), my = 0.5f * (y0 + y1);
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    float off = gk_rs(g) * jag * len;
    float nx = -dy, ny = dx;
    if (len > 1e-5f) { nx /= len; ny /= len; }
    mx += nx * off; my += ny * off;
    float tm = 0.5f * (ta + tb);
    gk_bolt_channel(g, b, x0, y0, mx, my, jag, depth - 1, ta, tm, wgt, px, py, np);
    gk_bolt_channel(g, b, mx, my, x1, y1, jag, depth - 1, tm, tb, wgt, px, py, np);
}
/* channel plus nbr branches hung off it (recursively fewer, thinner,
 * shorter).  blen = branch length as a fraction of the distance left to
 * the channel's end.  Branch arc-times start where they leave the parent. */
static void gk_bolt_tree(gk *g, gk_bolt *b, float x0, float y0, float x1, float y1,
                         float jag, int depth, float ta, float tb, float wgt,
                         int nbr, float blen)
{
    float px[GK_MAXPTS], py[GK_MAXPTS];
    int np = 1, k;
    px[0] = x0; py[0] = y0;
    gk_bolt_channel(g, b, x0, y0, x1, y1, jag, depth, ta, tb, wgt, px, py, &np);
    if (depth < 2 || nbr <= 0 || np < 4) return;
    for (k = 0; k < nbr; k++) {
        int i = 1 + (int)(gk_rf(g) * (float)(np - 3));
        float ax = px[i], ay = py[i];
        float dx = px[i + 1] - px[i - 1], dy = py[i + 1] - py[i - 1];
        float ang = atan2f(dy, dx) + (gk_rf(g) < 0.5f ? 1.0f : -1.0f) * (0.3f + 0.6f * gk_rf(g));
        float rem = sqrtf((x1 - ax) * (x1 - ax) + (y1 - ay) * (y1 - ay));
        float bl = rem * blen * (0.5f + 0.7f * gk_rf(g));
        if (bl < 1e-5f) continue;
        float ti = ta + (tb - ta) * (float)i / (float)(np - 1);
        float tspan = (tb - ta) * (bl / (rem + 1e-3f)) * 1.1f;
        gk_bolt_tree(g, b, ax, ay, ax + cosf(ang) * bl, ay + sinf(ang) * bl,
                     jag * 1.15f, depth - 2, ti, ti + tspan, wgt * 0.5f,
                     nbr / 3, blen);
    }
}
static void gk_bolt_gen(gk *g, gk_bolt *b, float x0, float y0, float x1, float y1,
                        float jag, int depth, int nbr, float blen)
{
    b->n = 0;
    if (depth > 8) depth = 8;
    gk_bolt_tree(g, b, x0, y0, x1, y1, jag, depth, 0.0f, 1.0f, 1.0f, nbr, blen);
}
/* draw a bolt revealed to arc-time prog (0..1+, >1 = complete), with
 * overall amplitude amp; core/glow radii scale with segment weight. */
static void gk_bolt_draw_xfm(gk *g, const gk_bolt *b, float ox, float oy, float rot, float scl,
                             float mir, float prog, float amp, const float *c, float cr, float gr, float gi)
{
    int i;
    float col[3];
    float ca = cosf(rot) * scl, sa = sinf(rot) * scl;
    /* mir = -1 reflects the bolt's local y before rotating */
    for (i = 0; i < b->n; i++) {
        const gk_bseg *s = &b->s[i];
        if (s->t0 >= prog) continue;
        float x1 = s->x1, y1 = s->y1, f = 1.0f;
        if (s->t1 > prog) {
            f = (prog - s->t0) / (s->t1 - s->t0);
            x1 = s->x0 + (s->x1 - s->x0) * f;
            y1 = s->y0 + (s->y1 - s->y0) * f;
        }
        float wa = amp * (0.35f + 0.65f * s->wgt);
        col[0] = c[0] * wa; col[1] = c[1] * wa; col[2] = c[2] * wa;
        float ws = 0.5f + 0.5f * s->wgt;
        float sy0 = s->y0 * mir; y1 *= mir;
        gk_seg(g, ox + s->x0 * ca - sy0 * sa, oy + s->x0 * sa + sy0 * ca,
                  ox + x1 * ca - y1 * sa,     oy + x1 * sa + y1 * ca,
               col, cr * ws, gr * ws, gi);
    }
}
static void gk_bolt_draw_xf(gk *g, const gk_bolt *b, float ox, float oy, float rot, float scl,
                            float prog, float amp, const float *c, float cr, float gr, float gi)
{
    gk_bolt_draw_xfm(g, b, ox, oy, rot, scl, 1.0f, prog, amp, c, cr, gr, gi);
}
static void gk_bolt_draw(gk *g, const gk_bolt *b, float prog, float amp,
                         const float *c, float cr, float gr, float gi)
{
    gk_bolt_draw_xfm(g, b, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, prog, amp, c, cr, gr, gi);
}
/* n-fold kaleidoscope: the bolt (local space, origin = centre) is drawn at
 * n rotations, and mirrored copies too when mirror != 0 */
static void gk_bolt_kaleido(gk *g, const gk_bolt *b, float ox, float oy, float rot0, float scl,
                            int n, int mirror, float prog, float amp, const float *c,
                            float cr, float gr, float gi)
{
    int k;
    for (k = 0; k < n; k++) {
        float r = rot0 + GK_TAU * (float)k / (float)n;
        gk_bolt_draw_xfm(g, b, ox, oy, r, scl, 1.0f, prog, amp, c, cr, gr, gi);
        if (mirror) gk_bolt_draw_xfm(g, b, ox, oy, r, scl, -1.0f, prog, amp, c, cr, gr, gi);
    }
}
/* smooth envelope for a strike: ramps up over `up`, holds, fades over
 * `down`; age in frames.  Returns 0..1 with C1-ish smoothstep edges. */
static inline float gk_env(float age, float up, float hold, float down)
{
    if (age <= 0.0f) return 0.0f;
    if (age < up) { float u = age / up; return u * u * (3.0f - 2.0f * u); }
    age -= up;
    if (age < hold) return 1.0f;
    age -= hold;
    if (age < down) { float u = 1.0f - age / down; return u * u * (3.0f - 2.0f * u); }
    return 0.0f;
}
static inline float gk_smooth(float u)
{
    if (u <= 0.0f) return 0.0f;
    if (u >= 1.0f) return 1.0f;
    return u * u * (3.0f - 2.0f * u);
}

/* ---- coarse grids ------------------------------------------------- */
/* bilinear-upsample a gw x gh scalar grid (values ~0..1) onto the canvas
 * as colour: v -> lut[v*255] built per frame by the caller (256 x RGB). */
static void gk_grid_fill(gk *g, const float *grid, int gw, int gh, const float *lut)
{
    int cw = g->cw, ch = g->ch, x, y;
    float sx = (float)(gw - 1) / (float)cw, sy = (float)(gh - 1) / (float)ch;
    for (y = 0; y < ch; y++) {
        float fy = ((float)y + 0.5f) * sy; int iy = (int)fy; if (iy >= gh - 1) iy = gh - 2;
        float uy = fy - (float)iy;
        const float *r0 = grid + iy * gw, *r1 = r0 + gw;
        float *row = g->acc + ((size_t)y * (size_t)cw) * 3;
        for (x = 0; x < cw; x++) {
            float fx = ((float)x + 0.5f) * sx; int ix = (int)fx; if (ix >= gw - 1) ix = gw - 2;
            float ux = fx - (float)ix;
            float a = r0[ix] + (r0[ix + 1] - r0[ix]) * ux;
            float b = r1[ix] + (r1[ix + 1] - r1[ix]) * ux;
            float v = a + (b - a) * uy;
            int k = (int)(v * 255.0f); if (k < 0) k = 0; if (k > 255) k = 255;
            row[x * 3 + 0] += lut[k * 3 + 0];
            row[x * 3 + 1] += lut[k * 3 + 1];
            row[x * 3 + 2] += lut[k * 3 + 2];
        }
    }
}
/* build a 256-entry colour ramp: value v -> palette (base + v*span),
 * whitened by wt*v (hot ends go pale), amplitude gamma-shaped: amp*v^gam. */
static void gk_lut_ramp(float *lut, const uint32_t *pal, int base, int span,
                        float wt, float amp, float gam)
{
    int k;
    for (k = 0; k < 256; k++) {
        float v = (float)k / 255.0f;
        gk_col(pal, base + (int)(v * (float)span), wt * v, amp * powf(v, gam), lut + k * 3);
    }
}
/* fbm value noise on a grid, 3 octaves; scale = cells per grid unit */
static void gk_grid_fbm(float *grid, int gw, int gh, float scale, float tx, float ty, uint32_t k)
{
    int x, y;
    for (y = 0; y < gh; y++)
        for (x = 0; x < gw; x++) {
            float fx = (float)x * scale + tx, fy = (float)y * scale + ty;
            float v = 0.55f * gk_noise2(fx, fy, k)
                    + 0.30f * gk_noise2(fx * 2.1f + 7.3f, fy * 2.1f + 1.7f, k + 1)
                    + 0.15f * gk_noise2(fx * 4.3f + 3.1f, fy * 4.3f + 9.2f, k + 2);
            grid[y * gw + x] = v;
        }
}
/* ---- 3-D helper ---------------------------------------------------- */
/* rotate about Y then X, perspective-project onto canvas */
typedef struct { float cx, cy, sy, cy_, sx, cx_, dist, scale; } gk_cam;
static inline void gk_cam_set(gk_cam *c, gk *g, float ay, float ax, float dist, float scale)
{
    c->cx = (float)g->cw * 0.5f; c->cy = (float)g->ch * 0.5f;
    c->sy = sinf(ay); c->cy_ = cosf(ay);
    c->sx = sinf(ax); c->cx_ = cosf(ax);
    c->dist = dist; c->scale = scale * g->sc;
}
/* rotate about Y then X, perspective-project; depth out = 1/(dist+z) scaled
 * so ~1 at the origin, >1 nearer the viewer */
static inline void gk_project(const gk_cam *c, float x, float y, float z, float *ox, float *oy, float *depth)
{
    float x1 = x * c->cy_ + z * c->sy, z1 = -x * c->sy + z * c->cy_;
    float y2 = y * c->cx_ - z1 * c->sx, z2 = y * c->sx + z1 * c->cx_;
    float p = c->dist / (c->dist + z2);
    *ox = c->cx + x1 * p * c->scale;
    *oy = c->cy + y2 * p * c->scale;
    if (depth) *depth = p;
}
#endif
