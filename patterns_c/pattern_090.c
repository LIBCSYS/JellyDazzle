/* 090 Diamond Burst — one huge rhombus: a confetti-mosaic heart wrapped in
 * expanding concentric diamond bands, on near-black, with soft streaks escaping
 * the side vertices. Port of lab/patterns/090_diamond_burst/proto.py.
 * Repaint pattern: the rim bands are a 1-D LUT over the diamond norm, the
 * confetti is a per-frame 8x8-cell colour table, streaks are two 1-D LUTs. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P90_TAU 6.28318530718f
#define P90_D 118.0f
#define P90_DI 73.16f                   /* 0.62 * D */
#define P90_HSPAN 410.0f
#define P90_CX 22
#define P90_CY 17

static float p90_ptab[1024][3];
static float p90_sin[2048];
static float p90_rim[1024][3];
static float p90_conf[P90_CY][P90_CX][3];
static float p90_bx[1024];
static float p90_ay[512];
static int p90_inited;

/* Column geometry. px, |px|, the confetti column and the streak-LUT index are
 * pure functions of x, so they are built once per resolution rather than once
 * per pixel. Expressions are copied verbatim from the inner loop. */
#define P90_BLK 512
typedef struct { float px, ax; uint16_t jxc, kbx; } p90_col;
static p90_col *p90_cols;
static int p90_colw;
static p90_col p90_colblk[P90_BLK];      /* used only if the alloc fails */

static void p90_fillcols(p90_col *c, int x0, int n, float isx)
{
    int i;
    for (i = 0; i < n; i++) {
        float px = ((float)(x0 + i) + 0.5f) * isx - 160.0f;
        float ax = fabsf(px);
        int jxc = (int)(ax * 0.125f);
        int k = (int)((px + 160.0f) * 2.0f);
        if (jxc >= P90_CX) jxc = P90_CX - 1;
        if (k < 0) k = 0; if (k > 1023) k = 1023;
        c[i].px = px; c[i].ax = ax;
        c[i].jxc = (uint16_t)jxc; c[i].kbx = (uint16_t)k;
    }
}

static void p90_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p90_sin[i] = sinf((float)i * (P90_TAU / 2048.0f));
    for (i = 0; i < 512; i++)
        p90_ay[i] = expf(-((float)i * 0.25f) * 0.2f);
    p90_inited = 1;
}

static void p90_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 24.0f) mx = 24.0f;
        p90_ptab[i][0] = r / mx; p90_ptab[i][1] = g / mx; p90_ptab[i][2] = b / mx;
    }
}

static float p90_lsin(float a)
{
    return p90_sin[((int)(a * 325.949318f + 32768.5f)) & 2047];
}

void pattern_090(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float isx = 320.0f / (float)w, isy = 240.0f / (float)h;
    float rsh = t * 0.010f, hdrift = t * 0.0012f, cdrift = t * 0.0015f;
    float spark = t * 0.02f;
    int i, jx, jy, x, y, x0, blk;
    (void)sl; (void)seed;

    blk = 0;
    if (!p90_inited) p90_init();
    p90_buildpal(pal);

    if (p90_colw != w) {
        free(p90_cols);
        p90_cols = (p90_col *)malloc(sizeof(p90_col) * (size_t)w);
        if (p90_cols) p90_fillcols(p90_cols, 0, w, isx);
        p90_colw = p90_cols ? w : 0;
    }

    /* rim band LUT over the diamond norm m, 0.5 unit steps */
    for (i = 0; i < 1024; i++) {
        float m = (float)i * 0.5f;
        float ph = m * (1.0f / 16.0f) - rsh;
        float fl = floorf(ph), fr = ph - fl;
        float sv = p90_lsin(fr * 3.14159265f);
        float band = sv * sv;
        float hue = fl * 0.31f + hdrift;
        float bri = 0.30f + 0.70f * band;
        const float *c = p90_ptab[(int)(hue * P90_HSPAN + 65536.0f) & 1023];
        p90_rim[i][0] = c[0] * bri; p90_rim[i][1] = c[1] * bri;
        p90_rim[i][2] = c[2] * bri;
    }
    /* confetti cell colours */
    for (jy = 0; jy < P90_CY; jy++)
        for (jx = 0; jx < P90_CX; jx++) {
            double s = sin((double)jx * 12.9898 + (double)jy * 78.233) * 43758.5453;
            float hv = (float)(s - floor(s));
            float sp = 0.5f + 0.5f * p90_lsin(hv * 6.2832f + spark
                                              + (float)(jx + jy) * 0.4f);
            float bri = 0.25f + 0.75f * sp;
            float hue = hv * 0.85f + cdrift;
            const float *c = p90_ptab[(int)(hue * P90_HSPAN + 65536.0f) & 1023];
            p90_conf[jy][jx][0] = c[0] * bri;
            p90_conf[jy][jx][1] = c[1] * bri;
            p90_conf[jy][jx][2] = c[2] * bri;
        }
    blk = p90_cols ? w : P90_BLK;

    /* horizontal streak ripple over x in [-160,160) */
    for (i = 0; i < 1024; i++)
        p90_bx[i] = 0.4f + 0.3f * p90_lsin(((float)i * 0.5f - 160.0f) * 0.11f
                                           - t * 0.014f);

    for (y = 0; y < h; y++) {
        float py = ((float)y + 0.5f) * isy - 120.0f;
        float ay = fabsf(py);
        float m0 = ay * 1.45f;
        float fall;
        int ky = (int)(ay * 4.0f);
        uint32_t *row = fb + (long)y * w;
        int jyc = (int)(ay * 0.125f); if (jyc >= P90_CY) jyc = P90_CY - 1;
        fall = (ky < 512) ? p90_ay[ky] : 0.0f;
        for (x0 = 0; x0 < w; x0 += blk) {
        const p90_col *cc;
        int n = w - x0; if (n > blk) n = blk;
        if (p90_cols) cc = p90_cols + x0;
        else { p90_fillcols(p90_colblk, x0, n, isx); cc = p90_colblk; }
        for (i = 0; i < n; i++) {
            float ax = cc[i].ax;
            float m = ax + m0;
            float cr, cg, cb, e;
            int k, ir, ig, ib;
            x = x0 + i;

            if (m <= P90_DI) {
                int jxc = cc[i].jxc;
                cr = p90_conf[jyc][jxc][0];
                cg = p90_conf[jyc][jxc][1];
                cb = p90_conf[jyc][jxc][2];
            } else if (m < P90_D) {
                k = (int)(m * 2.0f); if (k > 1023) k = 1023;
                cr = p90_rim[k][0]; cg = p90_rim[k][1]; cb = p90_rim[k][2];
            } else {
                float st = fall;
                if (st > 0.0f) st *= p90_bx[cc[i].kbx];
                else st = 0.0f;
                cr = 0.03f + st * 0.25f;
                cg = 0.03f + st * 0.45f;
                cb = 0.03f + st * 0.55f;
            }

            /* bright silhouette edges at the two diamond boundaries */
            e = 1.0f - fabsf(m - P90_D) * (1.0f / 2.2f);
            if (e > 0.0f) { cr += e * 0.32f; cg += e * 0.80f; cb += e * 0.80f; }
            e = 1.0f - fabsf(m - P90_DI) * (1.0f / 2.2f);
            if (e > 0.0f) { cr += e * 0.80f; cg += e * 0.40f; cb += e * 0.80f; }

            ir = (int)(cr * 255.0f); if (ir > 255) ir = 255; if (ir < 0) ir = 0;
            ig = (int)(cg * 255.0f); if (ig > 255) ig = 255; if (ig < 0) ig = 0;
            ib = (int)(cb * 255.0f); if (ib > 255) ib = 255; if (ib < 0) ib = 0;
            row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                   | ((uint32_t)ig << 8) | (uint32_t)ib;
        }
        }
    }
}
