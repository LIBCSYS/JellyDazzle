/* 640 Nova Fractal (fractal) — Newton's method with a push, and the islands it grows.
 *
 * Newton on z^p - 1 steps z -> z - f/f' and every start lands on a root: 631
 * draws those territories.  Nova (Paul Derbyshire, 1990s) adds a constant to
 * every step, z -> z - a·f(z)/f'(z) + c, and that one term changes everything.
 * Start each orbit at z = 1 and let c be the pixel — the Mandelbrot way of
 * asking the question — and the plane fills with smooth concentric ripples
 * (how fast the orbit settles) broken by little islands that are pure
 * Mandelbrot: copies of the set, sprouting from the ripples, where the orbit
 * never settles at all.
 *
 * The relaxation constant `a` is not held at 1.  It drifts on a small complex
 * loop, and any imaginary part twists the ripples into SPIRALS that wind and
 * unwind around the islands.  The view breathes in and out over one of four
 * hand-picked windows; most of the plane is either one flat basin or bare
 * noise, and the band worth watching is thin.
 *
 * Cubic only.  Degrees 4 and 5 were measured too: a quarter of the island
 * edge density of the cubic, which is where all the detail lives.  The seed
 * picks which view, the ripple density, the size and phase of `a`'s loop, the
 * framing tilt and where on the ramp it starts.
 *
 * COST STRATEGY.  The same coarse-grid, band-swept canvas as 629/631,
 * crossfaded by _xfade.h so a sweep never shows half-built — but with a finer
 * three-pixel cell, because the island filigree is the whole point and five
 * pixels blurs it into blobs.  The cubic is hard-coded (z^3 and z^2 are three
 * multiplies).  Ripple pixels settle in ten to twenty steps; only the islands
 * run to the cap of 30.
 */
#include "_spark572.h"
#include "_xfade.h"

#define NV640_CELL 3.0f
#define NV640_BAND 15
#define NV640_IT   30

static gk       g640;
static gk_xf xf640;
static uint32_t bs640 = 0xFFFFFFFFu;
static int      base640;
static int      row640;
static float    phi640;           /* clock for zoom, drift and `a`       */
static float    vx640, vy640, vh640;   /* view centre and half-height    */
static float    rot640;
static float    dens640;          /* ripple bands per palette lap        */
static float    wand640;          /* size of `a`'s loop around 1         */

/* Views worth watching: { centre x, centre y, half-height }.  Chosen by
 * scoring a grid of candidate windows for island-edge density (with the
 * island area capped so it is not mostly wash) and then looking at the best. */
static const float V640[4][3] = {
    { -0.45f, 0.47f, 0.16f },     /* a chain of buds across a ripple field */
    { -0.33f, 0.36f, 0.12f },     /* the filament spine, edge on            */
    { -0.45f, 0.40f, 0.22f },     /* wider: buds, spine and one big basin   */
    { -0.43f, 0.45f, 0.08f },     /* close in on the bud chain              */
};

void pattern_640(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int gx, gy, it;
    gk_setup(&g640, w, h);
    if (row640 == 0) gk_clear(&g640);

    float cw = (float)g640.cw, ch = (float)g640.ch, sc = g640.sc;

    if (seed != bs640) {
        base640 = (int)(seed & 0x7FFFu);
        const float *v = V640[(int)(gk_hash(seed + 42u) * 3.999f)];
        vx640   = v[0]; vy640 = v[1]; vh640 = v[2];
        phi640  = gk_hash(seed + 43u) * GK_TAU;
        rot640  = (gk_hash(seed + 44u) - 0.5f) * 0.9f;
        dens640 = 0.035f + 0.045f * gk_hash(seed + 45u);
        wand640 = 0.10f + 0.16f * gk_hash(seed + 46u);
        row640  = 0;
        gk_clear(&g640);
        gk_xf_reset(&xf640);
        bs640 = seed;
    }

    float cell  = NV640_CELL * sc;
    int   gcols = (int)(cw / cell) + 1;
    int   grows = (int)(ch / cell) + 1;

    float t   = (float)frame;
    float hh  = vh640 * (1.0f + 0.15f * sinf(phi640 * 0.47f));
    float ox  = vx640 + 0.06f * vh640 * sinf(phi640 * 0.31f);
    float oy  = vy640 + 0.06f * vh640 * cosf(phi640 * 0.23f);
    float rr  = rot640 + 0.20f * sinf(phi640 * 0.17f);
    float cs  = cosf(rr), sn = sinf(rr);
    float asp = cw / ch;

    /* `a` loops around 1; its imaginary part is what makes the spirals */
    float ar = 1.0f + wand640 * 0.5f * cosf(phi640);
    float ai =        wand640 * sinf(phi640 * 0.71f);

    float col[3];
    int   ylast = row640 + NV640_BAND;
    if (ylast > grows) ylast = grows;

    for (gy = row640; gy < ylast; gy++) {
        float py = ((float)gy + 0.5f) * cell;
        float v  = (py / ch - 0.5f) * 2.0f * hh;
        for (gx = 0; gx < gcols; gx++) {
            float px = ((float)gx + 0.5f) * cell;
            float u  = (px / cw - 0.5f) * 2.0f * hh * asp;

            float cr = u * cs - v * sn + ox;      /* c is the pixel        */
            float ci = u * sn + v * cs + oy;
            float zr = 1.0f, zi = 0.0f;           /* z starts at a root    */

            float last = 1.0f, trap = 1e18f;   /* trap: shortest step taken */
            for (it = 0; it < NV640_IT; it++) {
                float qr = zr * zr - zi * zi;         /* z^2               */
                float qi = 2.0f * zr * zi;
                float fr = qr * zr - qi * zi - 1.0f;  /* z^3 - 1           */
                float fi = qr * zi + qi * zr;
                float dr = 3.0f * qr, di = 3.0f * qi; /* 3 z^2             */
                float dd = dr * dr + di * di;
                if (dd < 1e-16f) break;

                float nr = (fr * dr + fi * di) / dd;   /* f / f'           */
                float ni = (fi * dr - fr * di) / dd;
                float sr = ar * nr - ai * ni - cr;     /* a·f/f' - c       */
                float si = ar * ni + ai * nr - ci;
                zr -= sr; zi -= si;
                last = sr * sr + si * si;
                if (last < trap) trap = last;
                if (last < 1e-10f) { it++; break; }
            }

            float hue, amp;
            if (it >= NV640_IT && last >= 1e-10f) {
                /* An island: the orbit never settled.  Black would punch a
                 * hole in the kaleidoscope and a flat wash reads as a dead
                 * patch, so the inside is lit by an orbit trap — the shortest
                 * step the orbit ever took varies smoothly in there and draws
                 * the island's own internal bulbs, as 629 does for Julia. */
                float lt = -logf(trap + 1e-12f);           /* about 0..27   */
                hue = 0.52f + lt * 0.022f;
                amp = 0.26f + 0.022f * lt;
                if (amp > 0.80f) amp = 0.80f;
            } else {
                /* smooth settle count, as 631: the last step's length turns
                 * the integer count continuous, and the density wraps it round
                 * the ramp several times — those wraps are the ripples */
                float sm = (float)it - logf(logf(last + 1e-30f) * -0.04f + 1.001f);
                if (sm < 0.0f) sm = 0.0f;
                float frac = sm * (1.0f / (float)NV640_IT);
                if (frac > 1.0f) frac = 1.0f;
                /* which of the three attractors it fell into, by angle —
                 * that is the two-tone split between neighbouring basins */
                float ang = atan2f(zi, zr);
                if (ang < 0.0f) ang += GK_TAU;
                int arm = (int)(ang * (3.0f / GK_TAU) + 0.5f) % 3;
                hue = (float)arm * 0.30f + sm * dens640;
                amp = 0.34f + 0.60f * sqrtf(frac);
            }
            sk_col(pal, sk_hidx(base640, hue + t * 0.00020f),
                   0.20f, 0.48f, amp, col);
            gk_dot(&g640, px, py, col, cell * 0.64f, cell * 0.92f, 0.24f);
        }
    }

    row640 = ylast;
    if (row640 >= grows) {
        row640 = 0;
        gk_xf_commit(&xf640, &g640, (grows + NV640_BAND - 1) / NV640_BAND);
        phi640 += 0.030f;                  /* a's loop about 35 s, zoom ~75 s */
    }

    gk_xf_present(&xf640, &g640, fb, w, h);
}
