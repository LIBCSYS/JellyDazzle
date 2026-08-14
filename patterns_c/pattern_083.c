/* 083 Patch Quilt — 48px patches, one of four motifs each, dark sashing and
 * glowing corner buttons. Port of lab/patterns/083_patch_quilt/proto.py.
 * Repaint pattern, full resolution, per-cell scalars + shared sine LUT. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P83_L 48.0f
#define P83_GX 8
#define P83_GY 7
#define P83_HSPAN 410.0f

static float p83_ptab[1024][3];
static float p83_sin[2048];
static float p83_ph[P83_GY][P83_GX];
static float p83_hue[P83_GY][P83_GX];
static int   p83_mid[P83_GY][P83_GX];
static int p83_inited;

/* Column geometry: the patch column, the offset within it and the two edge
 * distances are pure functions of x, so they are computed once per resolution
 * instead of once per pixel. Expressions are verbatim from the inner loop. */
#define P83_BLK 512
typedef struct { float u, au, ub; uint16_t cgx; } p83_col;
static p83_col *p83_cols;
static int p83_colw;
static p83_col p83_colblk[P83_BLK];       /* used only if the alloc fails */

static void p83_fillcols(p83_col *c, int x0, int n, float isx)
{
    int i;
    for (i = 0; i < n; i++) {
        float px = ((float)(x0 + i) + 0.5f) * isx + 8.0f;
        int cgx = (int)(px * (1.0f / P83_L));
        float u, au;
        if (cgx < 0) cgx = 0; if (cgx >= P83_GX) cgx = P83_GX - 1;
        u = px - (float)cgx * P83_L - P83_L * 0.5f;
        au = fabsf(u);
        c[i].u = u; c[i].au = au; c[i].ub = 24.0f - au;
        c[i].cgx = (uint16_t)cgx;
    }
}

static void p83_init(void)
{
    int i, gx, gy;
    for (i = 0; i < 2048; i++)
        p83_sin[i] = sinf((float)i * (6.28318530718f / 2048.0f));
    for (gy = 0; gy < P83_GY; gy++)
        for (gx = 0; gx < P83_GX; gx++) {
            double s = sin((double)gx * 127.1 + (double)gy * 311.7) * 43758.5453;
            double h = s - floor(s);
            p83_ph[gy][gx] = (float)(h * 6.2832);
            p83_hue[gy][gx] = (float)(h * 0.9);
            p83_mid[gy][gx] = (int)(h * 4.0);
            if (p83_mid[gy][gx] > 3) p83_mid[gy][gx] = 3;
        }
    p83_inited = 1;
}

static void p83_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 24.0f) mx = 24.0f;
        p83_ptab[i][0] = r / mx; p83_ptab[i][1] = g / mx; p83_ptab[i][2] = b / mx;
    }
}

static float p83_lsin(float a)
{
    return p83_sin[((int)(a * 325.949318f + 32768.5f)) & 2047];
}

void pattern_083(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float isx = 320.0f / (float)w, isy = 240.0f / (float)h;
    float a0 = -t * 0.016f, a1 = t * 0.014f, a2 = -t * 0.012f;
    float a3 = t * 0.010f, a4 = -t * 0.010f;
    float hrot = t * 0.0012f;
    float sashr, sashg, sashb, sm;
    float bh;
    int x, y, ir, ig, ib, x0, i, blk;
    (void)sl; (void)seed;

    if (!p83_inited) p83_init();
    p83_buildpal(pal);

    if (p83_colw != w) {
        free(p83_cols);
        p83_cols = (p83_col *)malloc(sizeof(p83_col) * (size_t)w);
        if (p83_cols) p83_fillcols(p83_cols, 0, w, isx);
        p83_colw = p83_cols ? w : 0;
    }

    blk = p83_cols ? w : P83_BLK;
    sm = 0.05f * p83_lsin(t * 0.01f);
    sashr = 0.10f + sm; sashg = 0.07f + sm; sashb = 0.18f + sm;
    bh = 0.12f + hrot;

    for (y = 0; y < h; y++) {
        float py = ((float)y + 0.5f) * isy + 8.0f;
        int cgy = (int)(py * (1.0f / P83_L));
        float v, av, vb;
        uint32_t *row = fb + (long)y * w;
        if (cgy < 0) cgy = 0; if (cgy >= P83_GY) cgy = P83_GY - 1;
        v = py - (float)cgy * P83_L - P83_L * 0.5f;
        av = fabsf(v);
        vb = 24.0f - av;                       /* distance to nearest v edge */
        for (x0 = 0; x0 < w; x0 += blk) {
        const p83_col *cc;
        int n = w - x0; if (n > blk) n = blk;
        if (p83_cols) cc = p83_cols + x0;
        else { p83_fillcols(p83_colblk, x0, n, isx); cc = p83_colblk; }
        for (i = 0; i < n; i++) {
            float u = cc[i].u, au = cc[i].au, ub = cc[i].ub;
            int cgx = cc[i].cgx;
            float fld, hue, bri, cr, cg, cb, ph;
            const float *c;
            x = x0 + i;
            ph = p83_ph[cgy][cgx];

            /* corner buttons sit on top of everything */
            if (au > 19.0f && av > 19.0f) {
                float bb = 0.9f + 0.1f * p83_lsin(t * 0.02f + ph);
                c = p83_ptab[(int)(bh * P83_HSPAN + 4096.0f) & 1023];
                cr = c[0] * bb; cg = c[1] * bb; cb = c[2] * bb;
                goto emit;
            }
            /* sashing between patches */
            if (ub < 2.5f || vb < 2.5f) {
                cr = sashr; cg = sashg; cb = sashb;
                goto emit;
            }

            switch (p83_mid[cgy][cgx]) {
            case 0:
                fld = 0.5f + 0.5f * p83_lsin((au + av) * 0.30f + a0 + ph);
                break;
            case 1:
                fld = 0.5f + 0.5f * p83_lsin(sqrtf(u * u + v * v) * 0.35f + a1 + ph);
                break;
            case 2:
                fld = 0.5f + 0.5f * p83_lsin((au < av ? au : av) * 0.55f + a2 + ph);
                break;
            default:
                fld = 0.5f + 0.5f * (p83_lsin(u * 0.45f + a3 + ph)
                                     * p83_lsin(v * 0.45f + a4));
                break;
            }
            hue = p83_hue[cgy][cgx] + fld * 0.22f + hrot;
            bri = 0.30f + 0.70f * fld;
            c = p83_ptab[(int)(hue * P83_HSPAN + 4096.0f) & 1023];
            cr = c[0] * bri; cg = c[1] * bri; cb = c[2] * bri;
        emit:
            ir = (int)(cr * 255.0f); if (ir > 255) ir = 255; if (ir < 0) ir = 0;
            ig = (int)(cg * 255.0f); if (ig > 255) ig = 255; if (ig < 0) ig = 0;
            ib = (int)(cb * 255.0f); if (ib > 255) ib = 255; if (ib < 0) ib = 0;
            row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                   | ((uint32_t)ig << 8) | (uint32_t)ib;
        }
        }
    }
}
