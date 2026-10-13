/* 641 Neon Mandel (fractal) — the boundary of the Mandelbrot set as glowing
 * filaments on black.
 *
 * Escape-time colouring paints the whole plane, which makes a fine ground and
 * a useless overlay: there is nothing dark to see through.  The DISTANCE
 * ESTIMATE does the opposite.  Carrying the derivative along the orbit,
 *     dz <- 2 z dz + 1,   d ~ |z| ln|z| / |dz|,
 * gives every escaping pixel its distance to the set in plane units.  Divide
 * by the size of a pixel and it becomes "how many pixels from the edge", and a
 * glow that falls off with that number draws the boundary itself — every
 * filament, spiral and minibrot antenna — as a thin line of light.  Away from
 * the edge the picture is black, so the layers below show through.
 *
 * The view tours one hand-picked point per appearance: zoom in from the whole
 * set to a few ten-thousandths, then back out, forever, on a cosine so speed
 * eases to zero at both ends.  The centre slides from the set's middle onto the
 * target as the zoom deepens (offset falls with the square of the span, so the
 * target walks into the middle instead of racing past).  Deepest span 1e-4 is
 * nowhere near float64 trouble (§2.4: ~1e-13).
 *
 * COST.  An internal plane of at most 400 px a side, computed a band of rows
 * per frame; each finished sweep is crossfaded with the one before (the
 * _xfade.h idea, 3.6.1), so a half-built picture is never on screen.  The
 * view is frozen for the length of a sweep, which is ~0.15 s — the zoom moves
 * ~1% in that time, too little to ghost.  The very first picture is a quick
 * coarse pass in one call, so the layer never opens on black.  Colour is
 * looked up per frame from a 1024-step LUT of `pal`, so C and the engine's
 * palette walk land immediately even though the geometry is a sweep old.
 *
 * Seed picks: the target point, how deep the dive goes, where in the dive it
 * starts, the rotation and its direction, the dive speed, the ramp start and
 * how fast hue runs along the filament.
 */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define P641_MAXS   400          /* internal plane, long side             */
#define P641_SWEEP  9            /* nominal frames per full sweep         */
#define P641_WORK   700000L      /* iterations per frame (~2 ms on an M5) */
#define P641_LUTN   1024         /* colour steps along the filament       */
#define P641_FAR    3.2          /* span of the wide view (whole set)     */

/* Boundary points worth diving into — each with the deepest span that still
 * shows structure there. The wide view is centred on the set (-0.6, 0). */
static const struct { double cx, cy, deep; } P641_SPOT[] = {
    { -0.743643887037151,  0.131825904205330, 0.00012 },  /* seahorse valley  */
    { -0.748,               0.100,            0.0060  },  /* seahorse spirals */
    {  0.2929859127507,     0.6117848324958,  0.00035 },  /* double spiral    */
    { -1.7496200000000,     0.0,              0.00020 },  /* minibrot on spine*/
    { -0.10109636384562,    0.95628651080914, 0.00080 },  /* filament tangle  */
    { -1.25066,             0.02012,          0.00040 },  /* dendrites        */
    {  0.432539867562512,   0.226118675951765,0.00035 },  /* spiral galaxy    */
    {  0.2549870375144766, -0.0005679790528465,0.00030 }, /* elephant valley  */
};
#define P641_NSPOT ((int)(sizeof P641_SPOT / sizeof P641_SPOT[0]))

static uint32_t p641_seed = 0xFFFFFFFFu;
static int      p641_w, p641_h, p641_iw, p641_ih;
static uint16_t *p641_li, *p641_ai, *p641_bi;   /* colour step: live, A, B   */
static uint8_t  *p641_lg, *p641_ag, *p641_bg;   /* glow 0..255: live, A, B   */
static uint8_t  *p641_img;                      /* RGB at internal res       */
static jd_up     p641_up;
static int      p641_have, p641_age, p641_row;
static int      p641_len = P641_SWEEP, p641_took;  /* crossfade length, frames */
static long     p641_work;                       /* iterations spent         */
static double   p641_view[6];                   /* frozen for one sweep      */
static double   p641_ph0, p641_spd, p641_rot0, p641_rdir, p641_depth;
static double   p641_hue;                       /* LUT steps per escape unit */
static int      p641_spot, p641_base;
static int      p641_fresh = 1;                 /* geometry changed: re-plan */

static float p641_h01(uint32_t x)
{
    x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16;
    return (float)(x >> 8) * (1.0f / 16777216.0f);
}

/* View at time t: span on a cosine dive, centre sliding onto the target. */
static void p641_plan(double t)
{
    double ph   = p641_ph0 + t * p641_spd;
    double dive = 0.5 - 0.5 * cos(ph);                 /* 0 wide .. 1 deep    */
    double span = P641_FAR * exp(-p641_depth * dive);
    double s    = span / P641_FAR; s *= s;
    double tx = P641_SPOT[p641_spot].cx, ty = P641_SPOT[p641_spot].cy;
    double rot = p641_rot0 + p641_rdir * t * 0.00042;
    p641_view[0] = tx + (-0.6 - tx) * s;
    p641_view[1] = ty + ( 0.0 - ty) * s;
    p641_view[2] = span * 0.5;
    p641_view[3] = cos(rot);
    p641_view[4] = sin(rot);
    /* deeper needs more iterations to resolve the edge: 90 wide .. ~330 deep */
    p641_view[5] = 90.0 + 26.0 * log(P641_FAR / span);
}

/* Compute rows y0..y1 of the live planes at `step` px pitch (1 = full res). */
static void p641_rows(int y0, int y1, int step, int itcap)
{
    double cx = p641_view[0], cy = p641_view[1], half = p641_view[2];
    double cs = p641_view[3], sn = p641_view[4];
    int    its = (int)p641_view[5];
    if (itcap && its > itcap) its = itcap;
    int iw = p641_iw, ih = p641_ih;
    double asp = (double)iw / (double)ih;
    double ps  = 2.0 * half / (double)ih;              /* plane units per px  */
    double inv = 1.0 / ps;
    for (int y = y0; y < y1; y += step) {
        double v = (((double)y + 0.5) / (double)ih - 0.5) * 2.0 * half;
        for (int x = 0; x < iw; x += step) {
            double u  = (((double)x + 0.5) / (double)iw - 0.5) * 2.0 * half * asp;
            double pr = cx + (u * cs - v * sn);
            double pi = cy + (u * sn + v * cs);
            int g = 0, k = 0;
            /* main cardioid and period-2 disc are inside: skip them exactly */
            double q = (pr - 0.25) * (pr - 0.25) + pi * pi;
            if (q * (q + pr - 0.25) <= 0.25 * pi * pi
                || (pr + 1.0) * (pr + 1.0) + pi * pi <= 0.0625) goto store;
            {
                double zr = 0.0, zi = 0.0, dr = 0.0, di = 0.0;
                double zr2 = 0.0, zi2 = 0.0, pr0 = 0.0, pi0 = 0.0;
                int i = 0;
                while (i < its && zr2 + zi2 <= 1.0e6) {
                    /* dz <- 2 z dz + 1, before z moves */
                    double ndr = 2.0 * (zr * dr - zi * di) + 1.0;
                    di = 2.0 * (zr * di + zi * dr);
                    dr = ndr;
                    zi = 2.0 * zr * zi + pi;
                    zr = zr2 - zi2 + pr;
                    zr2 = zr * zr; zi2 = zi * zi;
                    i++;
                    /* periodicity: an orbit that returns to where it was is
                     * caught in a cycle — interior, stop paying for it */
                    if ((i & 31) == 0) { pr0 = zr; pi0 = zi; }
                    else if (fabs(zr - pr0) + fabs(zi - pi0) < 1e-13) { i = its; break; }
                }
                p641_work += i;
                if (i >= its) goto store;                  /* inside: dark    */
                double m  = sqrt(zr2 + zi2);
                double dm = sqrt(dr * dr + di * di);
                double d  = m * log(m) / (dm > 1e-300 ? dm : 1e-300);
                double nd = d * inv * 1.6;                 /* px from the edge*/
                /* thin bright core, long faint halo, black beyond */
                double gl = 1.0 / (1.0 + nd * nd * (0.55 + 0.45 * nd));
                g = (int)(gl * 255.0 + 0.5);
                double nu = (double)i + 1.0 - log(log(m) * 1.442695) * 1.442695;
                k = (int)(nu * p641_hue) & (P641_LUTN - 1);
            }
        store:
            for (int yy = y; yy < y + step && yy < ih; yy++)
                for (int xx = x; xx < x + step && xx < iw; xx++) {
                    size_t o = (size_t)yy * iw + xx;
                    p641_lg[o] = (uint8_t)g; p641_li[o] = (uint16_t)k;
                }
        }
    }
}

static void p641_commit(int len)
{
    size_t n = (size_t)p641_iw * p641_ih;
    uint16_t *ti = p641_ai; p641_ai = p641_bi; p641_bi = ti;
    uint8_t  *tg = p641_ag; p641_ag = p641_bg; p641_bg = tg;
    memcpy(p641_bi, p641_li, n * sizeof *p641_li);
    memcpy(p641_bg, p641_lg, n);
    if (p641_have < 2) p641_have++;
    p641_age = 0; p641_len = len > 1 ? len : 1;
}

static int p641_alloc(int w, int h)
{
    int iw, ih;
    if (w >= h) { iw = w < P641_MAXS ? w : P641_MAXS; ih = (int)((long)iw * h / w); }
    else        { ih = h < P641_MAXS ? h : P641_MAXS; iw = (int)((long)ih * w / h); }
    if (iw < 8) iw = 8;
    if (ih < 8) ih = 8;
    size_t n = (size_t)iw * ih;
    free(p641_li); free(p641_ai); free(p641_bi);
    free(p641_lg); free(p641_ag); free(p641_bg); free(p641_img);
    p641_li = malloc(n * 2); p641_ai = malloc(n * 2); p641_bi = malloc(n * 2);
    p641_lg = malloc(n); p641_ag = malloc(n); p641_bg = malloc(n);
    p641_img = malloc(n * 3);
    p641_w = w; p641_h = h; p641_iw = iw; p641_ih = ih;
    if (!p641_li || !p641_ai || !p641_bi || !p641_lg || !p641_ag || !p641_bg || !p641_img) {
        p641_w = 0; return 0;
    }
    return 1;
}

void pattern_641(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (w != p641_w || h != p641_h) {
        if (!p641_alloc(w, h)) { memset(fb, 0, (size_t)w * h * 4); return; }
        p641_fresh = 1;
    }
    if (seed != p641_seed || p641_fresh) {
        p641_fresh = 0;
        p641_spot  = (int)(p641_h01(seed + 1u) * P641_NSPOT) % P641_NSPOT;
        double deep = P641_SPOT[p641_spot].deep * (0.6 + 1.6 * p641_h01(seed + 2u));
        p641_depth = log(P641_FAR / deep);
        p641_ph0   = p641_h01(seed + 3u) * 6.2831853;   /* where in the dive   */
        p641_spd   = 0.00085 + 0.00075 * p641_h01(seed + 4u);   /* 70..120 s a dive */
        p641_rot0  = p641_h01(seed + 5u) * 6.2831853;
        p641_rdir  = p641_h01(seed + 6u) < 0.5 ? -1.0 : 1.0;
        p641_base  = (int)(seed & JD_PAL_MASK);
        p641_hue   = 1.5 + 3.5 * p641_h01(seed + 7u);   /* slow: speckle otherwise */
        p641_seed  = seed;
        p641_have  = 0; p641_age = 0; p641_row = 0; p641_took = 0;
        /* the opening picture, coarse and in one go: never open on black */
        p641_plan((double)frame);
        p641_rows(0, p641_ih, 3, 70);
        p641_commit(P641_SWEEP);
        p641_plan((double)frame);
    }

    /* The next rows of the full-resolution sweep, until this frame's
     * iteration budget is spent. A fixed band cost 2 ms on average but spiked
     * past 10 ms where a deep view crosses a minibrot (every pixel to the cap);
     * a budget keeps the frame flat and lets that sweep take a little longer.
     * The crossfade then runs over however long the sweep actually took. */
    p641_work = 0;
    p641_took++;
    while (p641_row < p641_ih && p641_work < P641_WORK) {
        p641_rows(p641_row, p641_row + 1, 1, 0);
        p641_row++;
    }
    if (p641_row >= p641_ih) {
        p641_commit(p641_took);
        p641_row = 0; p641_took = 0;
        p641_plan((double)frame);                       /* next sweep's view   */
    }

    /* colour LUT for this frame: the filament hue walks the live ramp, and
     * the brightest channel is lifted so a dark ramp stretch still glows */
    static uint32_t lut[P641_LUTN];
    uint32_t drift = (uint32_t)frame * 3u;
    for (int i = 0; i < P641_LUTN; i++) {
        uint32_t c = pal[((uint32_t)p641_base + (uint32_t)i * 32u + drift) & JD_PAL_MASK];
        int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
        int mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
        if (mx < 190) {
            if (mx < 8) { r = g = b = 190; }
            else { r = r * 190 / mx; g = g * 190 / mx; b = b * 190 / mx; }
        }
        lut[i] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    /* crossfade the last two finished sweeps, linearly over one sweep */
    int f = p641_have < 2 ? 256 : p641_age * 256 / p641_len;
    if (f > 256) f = 256;
    if (p641_age < p641_len) p641_age++;
    int gq = 256 - f;
    size_t n = (size_t)p641_iw * p641_ih;
    uint8_t *o = p641_img;
    for (size_t i = 0; i < n; i++, o += 3) {
        int ga = p641_ag[i] * gq, gb = p641_bg[i] * f;
        if ((ga | gb) == 0) { o[0] = o[1] = o[2] = 0; continue; }
        uint32_t ca = lut[p641_ai[i]], cb = lut[p641_bi[i]];
        int r = (((ca >> 16) & 255) * ga + ((cb >> 16) & 255) * gb) >> 16;
        int g = (((ca >> 8) & 255) * ga + ((cb >> 8) & 255) * gb) >> 16;
        int b = ((ca & 255) * ga + (cb & 255) * gb) >> 16;
        int gl = (ga + gb) >> 8;                        /* 0..255 glow          */
        int wh = gl > 225 ? (gl - 225) * 3 : 0;         /* white-hot core, thin */
        r += wh; g += wh; b += wh;
        o[0] = (uint8_t)(r > 255 ? 255 : r);
        o[1] = (uint8_t)(g > 255 ? 255 : g);
        o[2] = (uint8_t)(b > 255 ? 255 : b);
    }
    jd_up_blit(&p641_up, fb, w, h, p641_img, p641_iw, p641_ih);
}
