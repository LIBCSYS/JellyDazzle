/* 610 Flame IFS — Scott Draves' fractal flame, played as a chaos game.
 *
 * The other fractals here are DETERMINISTIC pictures of an equation. This one
 * is a game of chance that converges on a shape. Take a handful of affine maps,
 * pass each through a nonlinear "variation" — a swirl, a spherical inversion, a
 * horseshoe fold — then start with a random point and repeatedly: pick one of
 * the maps at random, apply it, plot where you land. The point wanders forever,
 * and the histogram of where it has been is the attractor. Draves' 1992 insight
 * was that logging the DENSITY of visits rather than plotting hits turns a
 * scratchy line drawing into something that looks lit from inside.
 *
 * Two reasons it earns a slot:
 *
 *   - The vocabulary is unlike everything else in the library. No escape
 *     boundaries, no basins, no rigging. Flames make plumes, whorls, drapes and
 *     wings — organic, smoky forms with soft interiors.
 *
 *   - The shape is not chosen from a list. The affine coefficients and the
 *     variation on each map are drawn from the seed, so every instance is a
 *     genuinely different attractor that has probably never been rendered
 *     before. That is the opposite of picking one of seven hand-tuned spots.
 *
 * Like 609 it EXPOSES rather than renders: a fixed budget of points per frame,
 * accumulating. It comes up out of the dark over a few seconds and keeps
 * gaining detail. Colour is carried along with the point — each map has its own
 * colour coordinate and the running value is blended toward it at every step,
 * so regions of the attractor built by different maps land in different parts
 * of the palette. Density then shades within that. Nothing is black.
 *
 * The one hazard of random coefficients is a degenerate attractor: maps that
 * expand instead of contract send the point to infinity, and maps that are too
 * contractive collapse it to a dot. Both are handled — the point is reset if it
 * stops being finite, and the coefficients are scaled toward contraction.
 */
#include "../engine/jellydazzle.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

#define F610_NMAP  4              /* affine maps in the system */
#define F610_SHOTS 26000          /* points plotted per frame  */
#define F610_WARM  24             /* discarded before plotting */

typedef struct {
    double a, b, c, d, e, f;      /* affine: x' = a x + b y + c, y' = d x + e y + f */
    int    var;                   /* which nonlinearity        */
    double col;                   /* colour coordinate         */
} f610_map;

static uint32_t *cnt610;
static float    *col610;
static int       pw610, ph610;
static uint32_t  seed610 = 0xFFFFFFFFu;
static uint32_t  rs610;
static uint32_t  peak610;
static f610_map  map610[F610_NMAP];
static double    px610, py610, pc610;
static double    par610[8];

static uint32_t r610(uint32_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
    return *s;
}
static double d610(uint32_t *s) { return (double)(r610(s) >> 8) * (1.0 / 16777216.0); }
static double s610(uint32_t *s) { return d610(s) * 2.0 - 1.0; }

/* The variations. These are the shapes; the affine part only positions them. */
static void var610(int v, double x, double y, double *ox, double *oy)
{
    double r2 = x * x + y * y;
    double r  = sqrt(r2);
    switch (v) {
    default:
    case 0:  *ox = x; *oy = y; break;                          /* linear      */
    case 1:  *ox = sin(x); *oy = sin(y); break;                /* sinusoidal  */
    case 2:  { double k = 1.0 / (r2 + 1e-9);
               *ox = x * k; *oy = y * k; } break;              /* spherical   */
    case 3:  { double s = sin(r2), c = cos(r2);
               *ox = x * s - y * c; *oy = x * c + y * s; } break; /* swirl    */
    case 4:  { double k = 1.0 / (r + 1e-9);
               *ox = (x - y) * (x + y) * k; *oy = 2.0 * x * y * k; } break; /* horseshoe */
    case 5:  { double t = atan2(x, y);
               *ox = t * 0.3183098862; *oy = r - 1.0; } break;  /* polar      */
    case 6:  { double t = atan2(x, y);
               *ox = r * sin(t + r); *oy = r * cos(t - r); } break; /* handkerchief */
    case 7:  { double t = atan2(x, y) * 0.3183098862;
               *ox = t * sin(3.14159265 * r); *oy = t * cos(3.14159265 * r); } break; /* disc */
    case 8:  { double t = atan2(x, y);
               *ox = sin(t) / (r + 1e-9); *oy = r * cos(t); } break; /* hyperbolic */
    case 9:  { double t = atan2(x, y);
               *ox = sin(t) * cos(r); *oy = cos(t) * sin(r); } break; /* diamond */
    }
}

static void plan610(int w, int h, uint32_t seed)
{
    size_t np = (size_t)w * h;
    if (pw610 != w || ph610 != h) {
        free(cnt610); free(col610);
        cnt610 = (uint32_t *)malloc(np * sizeof(uint32_t));
        col610 = (float *)malloc(np * sizeof(float));
        pw610 = w; ph610 = h;
    }
    if (!cnt610 || !col610) return;
    memset(cnt610, 0, np * sizeof(uint32_t));
    memset(col610, 0, np * sizeof(float));
    peak610 = 1;

    uint32_t s = seed ? seed : 0x1992F1A3u;
    rs610 = s | 1u;
    for (int i = 0; i < F610_NMAP; i++) {
        /* Scaled toward contraction. Free coefficients in [-1,1] make an
         * expanding system as often as not, and an expanding system throws the
         * point to infinity and plots nothing. */
        map610[i].a = s610(&s) * 0.85; map610[i].b = s610(&s) * 0.85;
        map610[i].d = s610(&s) * 0.85; map610[i].e = s610(&s) * 0.85;
        map610[i].c = s610(&s) * 0.60; map610[i].f = s610(&s) * 0.60;
        map610[i].var = (int)(d610(&s) * 10.0); if (map610[i].var > 9) map610[i].var = 9;
        map610[i].col = d610(&s);
    }
    px610 = s610(&s) * 0.5; py610 = s610(&s) * 0.5; pc610 = d610(&s);

    par610[0] = 8.0 + d610(&s) * 220.0;      /* palette base              */
    par610[1] = 0.42 + d610(&s) * 0.50;      /* view scale                */
    par610[2] = d610(&s) * 6.28318530718;    /* view rotation             */
    par610[3] = 60.0 + d610(&s) * 90.0;      /* density contrast          */
    par610[4] = 40.0 + d610(&s) * 120.0;     /* colour spread             */
}

static void expose610(int w, int h)
{
    if (!cnt610 || !col610) return;
    double scale = par610[1];
    double cs = cos(par610[2]), sn = sin(par610[2]);
    double halfy = 2.2 * scale;
    double halfx = halfy * (double)w / (double)h;

    double x = px610, y = py610, c = pc610;
    for (int n = 0; n < F610_SHOTS; n++) {
        const f610_map *m = &map610[r610(&rs610) & (F610_NMAP - 1)];
        double ax = m->a * x + m->b * y + m->c;
        double ay = m->d * x + m->e * y + m->f;
        double vx, vy;
        var610(m->var, ax, ay, &vx, &vy);
        x = vx; y = vy;
        c = (c + m->col) * 0.5;

        /* A variation can produce inf or NaN (spherical at the origin, polar on
         * the axis). Once that happens every later point is poisoned, so the
         * game is restarted rather than left to plot nothing. */
        if (!(x > -1e6 && x < 1e6 && y > -1e6 && y < 1e6)) {
            x = s610(&rs610) * 0.5; y = s610(&rs610) * 0.5; c = d610(&rs610);
            continue;
        }
        if (n < F610_WARM) continue;        /* let it settle onto the attractor */

        double rx = x * cs - y * sn;
        double ry = x * sn + y * cs;
        int ix = (int)((rx / halfx * 0.5 + 0.5) * (double)w);
        int iy = (int)((ry / halfy * 0.5 + 0.5) * (double)h);
        if (ix < 0 || ix >= w || iy < 0 || iy >= h) continue;
        size_t o = (size_t)iy * w + ix;
        uint32_t k = ++cnt610[o];
        col610[o] += (float)c;
        if (k > peak610) peak610 = k;
    }
    px610 = x; py610 = y; pc610 = c;
}

void pattern_610(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (seed != seed610 || pw610 != w || ph610 != h) {
        plan610(w, h, seed);
        seed610 = seed;
    }
    if (!cnt610 || !col610) return;
    expose610(w, h);

    int    base   = (int)par610[0];
    double contr  = par610[3];
    double spread = par610[4];
    double norm   = contr / log((double)peak610 + 1.0);

    uint32_t rot = (uint32_t)frame * 12u;
    uint32_t lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = pal[((uint32_t)i * 128u + rot) & JD_PAL_MASK];

    /* An attractor occupies a fraction of the frame; everywhere else the
     * density is zero and lands on one index — a flat wall of colour behind the
     * flame. A slow separable wash gives that emptiness a gentle drifting
     * gradient instead. Separable so it costs two small tables per frame rather
     * than a transcendental per pixel. */
    double t = (double)frame * 0.006;
    static float wx[4096], wy[4096];
    int wlim = w < 4096 ? w : 4096, hlim = h < 4096 ? h : 4096;
    for (int i = 0; i < wlim; i++) {
        double u = (double)i / (double)w;
        wx[i] = (float)(9.0 * sin(u * 4.1 + t) + 5.0 * sin(u * 9.3 - t * 0.7));
    }
    for (int i = 0; i < hlim; i++) {
        double v = (double)i / (double)h;
        wy[i] = (float)(9.0 * sin(v * 3.3 - t * 0.8) + 5.0 * sin(v * 7.7 + t * 0.5));
    }

    for (int y = 0; y < h; y++) {
        float wyv = wy[y < hlim ? y : hlim - 1];
        for (int x = 0; x < w; x++) {
            size_t i = (size_t)y * w + x;
            uint32_t k = cnt610[i];
            /* Colour comes from WHICH maps built this pixel; density shades it. */
            double cc = k ? (double)col610[i] / (double)k : 0.0;
            int idx = base + (int)(cc * spread) + (int)(log((double)k + 1.0) * norm)
                    + (int)(wyv + wx[x < wlim ? x : wlim - 1]);
            fb[i] = lut[idx & 255];
        }
    }
}
