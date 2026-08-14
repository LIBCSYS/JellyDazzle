/* 185 Sandpile — the abelian sandpile, grown live. Grains are dropped on the
 * centre cell of a 181x181 lattice; any cell holding four or more topples,
 * sending one grain to each of its four neighbours, and the avalanche runs
 * until every cell is stable. The result is the famous theorem-with-a-picture:
 * a deterministic, perfectly D4-symmetric fractal mandala of triangular
 * patches, the same shape at every scale, with the pile radius growing as
 * sqrt(n). Heights 0..3 are the four colours; height 0 shows the bare glaze, so
 * the mandala reads as coloured lace on a surface rather than a filled disc.
 * Toppling is done with h>>2 multi-topples in a tracked active bounding box, and the
 * displayed field chases the integer heights with a 10-frame lag so a cell
 * changing level is a fade, never a flip. The lattice is sampled through a
 * slowly turning, slowly breathing transform, so the finished mandala keeps
 * drifting after the growth front has passed. Accumulator: at sl == 0 it clears
 * and pre-rolls 9000 grains in one abelian batch, then builds for the whole
 * segment with the zoom tracking the pile radius, so the mandala is full-frame
 * from the first frame and the growth reads as detail resolving. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p185_up;

#define CW 480
#define CH 360
#define GN 181
#define GC 90

static unsigned char p185_img[CW * CH * 3];
static int p185_h[GN * GN];
static float p185_d[GN * GN];
static int *p185_xm;
static int p185_xmw;
static int p185_x0, p185_x1, p185_y0, p185_y1;
static float p185_hue0, p185_huew, p185_rot, p185_zoom;
static uint32_t p185_seedc;
static int p185_ready, p185_last, p185_full;

/* --- framing and ground (v2.1) -------------------------------------------
 * Two measured defects.  (1) Height 0 and everything off-lattice were painted
 * 0,0,0, so the mandala was lace on absolute void: luma 0.00 at sl==0 and
 * still only 3.6 five seconds in.  (2) The zoom was a constant sized for the
 * FINISHED pile, so the first half of every segment was a small disc adrift
 * in that void — the pile radius grows as sqrt(n) and only fills the frame
 * near the end.  Fixed by pre-rolling a few thousand grains (the sandpile is
 * abelian, so dropping them in one batch and relaxing gives bit-identical
 * state to dropping them one per frame) and then letting the zoom TRACK the
 * pile radius, heavily damped, so the mandala fills the frame from sl==0
 * onward and the growth reads as detail resolving rather than a dot swelling. */
#define P185_PRE   9000      /* grains dropped at reset (8.5 ms, no frame drop) */
#define P185_FILL  140.0f     /* screen half-height the pile radius maps to */
#define P185_GB    17.0f      /* ground brightness                          */
static unsigned char p185_vig[CW * CH];
static unsigned char p185_gt[256][3];   /* glaze by vignette level, per frame */
static int   p185_vready = 0;
static float p185_sc;         /* damped zoom, tracks the pile radius        */

static void p185_mkvig(void)
{
    int x, y;
    for (y = 0; y < CH; y++)
        for (x = 0; x < CW; x++) {
            float dx = ((float)x - CW * 0.5f) / (CW * 0.5f);
            float dy = ((float)y - CH * 0.5f) / (CH * 0.5f);
            float v = 1.0f - 0.46f * (dx * dx + dy * dy);
            if (v < 0.10f) v = 0.10f;
            p185_vig[y * CW + x] = (unsigned char)lrintf(v * 255.0f);
        }
    p185_vready = 1;
}

/* Half-width of the toppled region, in lattice cells. */
static float p185_radius(void)
{
    int a = p185_x1 - GC, b = GC - p185_x0;
    int c = p185_y1 - GC, d = GC - p185_y0;
    if (b > a) a = b;
    if (c > a) a = c;
    if (d > a) a = d;
    return (float)(a < 2 ? 2 : a);
}

static uint32_t p185_rs;
static float p185_rf(void)
{
    p185_rs ^= p185_rs << 13; p185_rs ^= p185_rs >> 17; p185_rs ^= p185_rs << 5;
    return (float)(p185_rs >> 8) * (1.0f / 16777216.0f);
}

static void p185_pal3(const uint32_t *pal, float hue, float sat, float *o)
{
    uint32_t p; float r, g, b, mx;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
    mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    o[0] = (1.0f - sat) + sat * r / mx;
    o[1] = (1.0f - sat) + sat * g / mx;
    o[2] = (1.0f - sat) + sat * b / mx;
}

static int p185_sweep(void);          /* defined below; used by p185_reset */

static void p185_reset(uint32_t seed)
{
    p185_rs = seed ? seed ^ 0x5A11D0u : 0x5A11D0u;
    p185_rf(); p185_rf();
    memset(p185_h, 0, sizeof p185_h);
    memset(p185_d, 0, sizeof p185_d);
    p185_x0 = p185_y0 = GC - 1; p185_x1 = p185_y1 = GC + 1;
    p185_hue0 = p185_rf();
    p185_huew = 0.26f + p185_rf() * 0.52f;
    p185_rot  = (p185_rf() < 0.5f ? -1.0f : 1.0f) * (0.00022f + p185_rf() * 0.00040f);
    p185_zoom = 0.00039f + p185_rf() * 0.00034f;
    p185_full = 0;
    p185_ready = 1;
    p185_seedc = seed;
    /* Abelian: one batch relaxed to stability == P185_PRE single drops. */
    p185_h[GC * GN + GC] += P185_PRE;
    {
        int guard = 20000;
        while (p185_sweep() && --guard) ;
    }
    /* Seed the display lag at the settled heights so sl==0 is the pile, not
     * a ten-frame fade up out of black. */
    {
        int i;
        for (i = 0; i < GN * GN; i++)
            p185_d[i] = (float)(p185_h[i] > 3 ? 3 : p185_h[i]);
    }
    p185_sc = p185_radius() / P185_FILL;
    if (p185_sc < 0.055f) p185_sc = 0.055f;
    if (p185_sc > 0.520f) p185_sc = 0.520f;
}

/* Returns the number of cells that toppled, so a pre-roll can relax the pile
 * to stability instead of guessing a sweep count. */
static int p185_sweep(void)
{
    int x, y, i, k, nt = 0;
    int nx0 = p185_x0, nx1 = p185_x1, ny0 = p185_y0, ny1 = p185_y1;
    for (y = p185_y0; y <= p185_y1; y++) {
        i = y * GN + p185_x0;
        for (x = p185_x0; x <= p185_x1; x++, i++) {
            if (p185_h[i] < 4) continue;
            nt++;
            k = p185_h[i] >> 2;
            p185_h[i] -= k << 2;
            p185_h[i - 1] += k; p185_h[i + 1] += k;
            p185_h[i - GN] += k; p185_h[i + GN] += k;
            if (x - 1 < nx0) nx0 = x - 1;
            if (x + 1 > nx1) nx1 = x + 1;
            if (y - 1 < ny0) ny0 = y - 1;
            if (y + 1 > ny1) ny1 = y + 1;
        }
    }
    if (nx0 < 2) { nx0 = 2; p185_full = 1; }
    if (ny0 < 2) { ny0 = 2; p185_full = 1; }
    if (nx1 > GN - 3) { nx1 = GN - 3; p185_full = 1; }
    if (ny1 > GN - 3) { ny1 = GN - 3; p185_full = 1; }
    p185_x0 = nx0; p185_x1 = nx1; p185_y0 = ny0; p185_y1 = ny1;
    return nt;
}

static void p185_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p185_xmw != w) {
        free(p185_xm);
        p185_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p185_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p185_xmw = w;
    }
    jd_up_blit(&p185_up, fb, w, h, p185_img, CW, CH);
}

void pattern_185(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, cs, sn, sc, col[4][3], gcol[3];
    int x, y, i, k;
    if (!p185_vready) p185_mkvig();
    if (!p185_ready || p185_seedc != seed || sl == 0 || sl < p185_last)
        p185_reset(seed);
    p185_last = sl;

    if (!p185_full) p185_h[GC * GN + GC] += 15;
    for (k = 0; k < 14; k++) p185_sweep();

    for (y = 1; y < GN - 1; y++) {
        i = y * GN + 1;
        for (x = 1; x < GN - 1; x++, i++) {
            float tv = (float)(p185_h[i] > 3 ? 3 : p185_h[i]);
            p185_d[i] += (tv - p185_d[i]) * 0.10f;
        }
    }

    for (k = 0; k < 4; k++)
        p185_pal3(pal, p185_hue0 + p185_huew * ((float)k * (1.0f / 3.0f)),
                  0.80f + 0.06f * (float)k, col[k]);

    /* Track the growing pile, damped hard enough that the zoom-out is a drift
     * and never a lurch: 0.008 per frame is a ~2 s time constant. */
    {
        float target = p185_radius() / P185_FILL;
        if (target < 0.055f) target = 0.055f;
        if (target > 0.520f) target = 0.520f;
        p185_sc += (target - p185_sc) * 0.008f;
    }
    sc = p185_sc * (1.0f + 0.052f * sinf(t * p185_zoom));
    cs = cosf(t * p185_rot) * sc; sn = sinf(t * p185_rot) * sc;
    /* Dark, desaturated glaze so height 0 and the off-lattice surround read as
     * a surface the mandala is drawn ON, not as a hole.  The glaze depends on
     * the pixel only through its 8-bit vignette level, so it collapses into a
     * 256-entry table built once per frame instead of three float multiplies
     * per pixel — that is the difference between 7.4 ms and 5.6 ms a frame. */
    p185_pal3(pal, p185_hue0 + 0.47f, 0.42f, gcol);
    for (k = 0; k < 256; k++) {
        float ge = P185_GB * ((float)k * (1.0f / 255.0f));
        p185_gt[k][0] = (unsigned char)(int)(gcol[0] * ge);
        p185_gt[k][1] = (unsigned char)(int)(gcol[1] * ge);
        p185_gt[k][2] = (unsigned char)(int)(gcol[2] * ge);
    }

    for (y = 0; y < CH; y++) {
        float dy = (float)y + 0.5f - CH * 0.5f;
        unsigned char *op = p185_img + y * CW * 3;
        const unsigned char *vg = p185_vig + y * CW;
        for (x = 0; x < CW; x++) {
            float dx = (float)x + 0.5f - CW * 0.5f;
            float gx = dx * cs - dy * sn + (float)GC;
            float gy = dx * sn + dy * cs + (float)GC;
            int ix = (int)gx, iy = (int)gy;
            float v, fx, fy, a, b, br, c0[3];
            const unsigned char *gt = p185_gt[vg[x]];
            int gr = gt[0], gg = gt[1], gb = gt[2];
            int lo;
            if (ix < 1 || iy < 1 || ix >= GN - 2 || iy >= GN - 2) {
                op[x * 3] = (unsigned char)gr; op[x * 3 + 1] = (unsigned char)gg;
                op[x * 3 + 2] = (unsigned char)gb; continue;
            }
            fx = gx - (float)ix; fy = gy - (float)iy;
            i = iy * GN + ix;
            a = p185_d[i] + (p185_d[i + 1] - p185_d[i]) * fx;
            b = p185_d[i + GN] + (p185_d[i + GN + 1] - p185_d[i + GN]) * fx;
            v = a + (b - a) * fy;
            if (v < 0.0f) v = 0.0f; else if (v > 2.999f) v = 2.999f;
            lo = (int)v;
            fx = v - (float)lo;
            c0[0] = col[lo][0] + (col[lo + 1 > 3 ? 3 : lo + 1][0] - col[lo][0]) * fx;
            c0[1] = col[lo][1] + (col[lo + 1 > 3 ? 3 : lo + 1][1] - col[lo][1]) * fx;
            c0[2] = col[lo][2] + (col[lo + 1 > 3 ? 3 : lo + 1][2] - col[lo][2]) * fx;
            br = v * (1.0f / 3.0f);
            br = br * (0.26f + 0.74f * br);
            {
                /* Mandala over the glaze: height 0 leaves the ground showing
                 * through untouched, so the lace reading survives. */
                int r = gr + (int)(c0[0] * br * 232.0f),
                    g = gg + (int)(c0[1] * br * 232.0f),
                    bl = gb + (int)(c0[2] * br * 232.0f);
                op[x * 3]     = (unsigned char)(r > 255 ? 255 : r);
                op[x * 3 + 1] = (unsigned char)(g > 255 ? 255 : g);
                op[x * 3 + 2] = (unsigned char)(bl > 255 ? 255 : bl);
            }
        }
    }
    p185_blit(fb, w, h);
}
