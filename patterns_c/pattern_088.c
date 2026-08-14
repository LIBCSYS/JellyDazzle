/* 088 Star Cross — Islamic star-and-cross tiling: gold-rimmed 8-point stars
 * filled with concentric rainbow layers on a dark indigo weave, each breathing
 * on its own hashed phase while the whole tiling drifts diagonally.
 * Port of lab/patterns/088_star_cross/proto.py. Repaint pattern, full
 * resolution; the per-cell scalars (hash, breathing radius, hue base) are
 * rebuilt into a small table once per frame, the inner loop is compares +
 * two table reads. */
#include "../jellydazzle.h"
#include <math.h>

#define P88_L 76.0f
#define P88_HL 38.0f
#define P88_TAU 6.28318530718f
#define P88_HSPAN 410.0f
#define P88_CN 16

static float p88_ptab[1024][3];
static float p88_sin[2048];
static float p88_gnd[1024][3];
static float p88_Rs[P88_CN][P88_CN];
static float p88_iRs[P88_CN][P88_CN];
static float p88_hb[P88_CN][P88_CN];
static float p88_bp[P88_CN][P88_CN];
static int p88_inited;

static void p88_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p88_sin[i] = sinf((float)i * (P88_TAU / 2048.0f));
    p88_inited = 1;
}

static void p88_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 24.0f) mx = 24.0f;
        p88_ptab[i][0] = r / mx; p88_ptab[i][1] = g / mx; p88_ptab[i][2] = b / mx;
    }
}

static float p88_lsin(float a)
{
    return p88_sin[((int)(a * 325.949318f + 32768.5f)) & 2047];
}

void pattern_088(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float isx = 320.0f / (float)w, isy = 240.0f / (float)h;
    float dxo = t * 0.03f, dyo = t * 0.018f;
    float hdrift = t * 0.0013f, gph = -t * 0.006f, bph = -t * 0.014f;
    float brt = t * 0.011f;
    int mbase, nbase, mi, ni, i, x, y;
    (void)sl; (void)seed;

    if (!p88_inited) p88_init();
    p88_buildpal(pal);

    /* ground weave LUT over (x+y) folded into 1024 slots of 0.5 units */
    for (i = 0; i < 1024; i++) {
        float g = 0.06f + 0.05f * (0.5f + 0.5f * p88_lsin((float)i * 0.5f * 0.15f + gph));
        p88_gnd[i][0] = g * 0.6f; p88_gnd[i][1] = g * 0.5f; p88_gnd[i][2] = g * 1.0f;
    }

    mbase = (int)floorf(dxo * (2.0f / P88_L)) - 2;
    nbase = (int)floorf(dyo * (2.0f / P88_L)) - 2;
    for (ni = 0; ni < P88_CN; ni++)
        for (mi = 0; mi < P88_CN; mi++) {
            double cx = (double)(mbase + mi) * (double)P88_HL;
            double cy = (double)(nbase + ni) * (double)P88_HL;
            double s = sin(cx * 0.113 + cy * 0.271) * 43758.5453;
            float ch = (float)(s - floor(s));
            float Rs = 27.0f + 4.0f * p88_lsin(brt + ch * 6.28f);
            p88_Rs[ni][mi] = Rs;
            p88_iRs[ni][mi] = 0.45f / Rs;
            p88_hb[ni][mi] = ch * 0.35f + hdrift;
            p88_bp[ni][mi] = bph + ch * 6.28f;
        }

    for (y = 0; y < h; y++) {
        float yy = ((float)y + 0.5f) * isy + dyo;
        int j1 = (int)floorf(yy * (1.0f / P88_L) + 0.5f);
        int j2 = (int)floorf((yy - P88_HL) * (1.0f / P88_L) + 0.5f);
        float c1y = (float)j1 * P88_L, c2y = (float)j2 * P88_L + P88_HL;
        float v1 = yy - c1y, v2 = yy - c2y;
        float q1y = v1 * v1, q2y = v2 * v2;
        uint32_t *row = fb + (long)y * w;
        for (x = 0; x < w; x++) {
            float xx = ((float)x + 0.5f) * isx + dxo;
            int i1 = (int)floorf(xx * (1.0f / P88_L) + 0.5f);
            int i2 = (int)floorf((xx - P88_HL) * (1.0f / P88_L) + 0.5f);
            float u1 = xx - (float)i1 * P88_L;
            float u2 = xx - ((float)i2 * P88_L + P88_HL);
            float u, v, au, av, cheb, diam, d8, Rs, cr, cg, cb;
            int m, n, k, ir, ig, ib;

            if (u1 * u1 + q1y <= u2 * u2 + q2y) {
                u = u1; v = v1; m = 2 * i1; n = 2 * j1;
            } else {
                u = u2; v = v2; m = 2 * i2 + 1; n = 2 * j2 + 1;
            }
            mi = m - mbase; if (mi < 0) mi = 0; if (mi >= P88_CN) mi = P88_CN - 1;
            ni = n - nbase; if (ni < 0) ni = 0; if (ni >= P88_CN) ni = P88_CN - 1;

            au = fabsf(u); av = fabsf(v);
            cheb = au > av ? au : av;
            diam = (au + av) * 0.70710678f;
            d8 = cheb < diam ? cheb : diam;
            Rs = p88_Rs[ni][mi];

            if (d8 < Rs) {
                float hue = d8 * p88_iRs[ni][mi] + p88_hb[ni][mi];
                float bri = 0.55f + 0.45f * p88_lsin(d8 * 0.35f + p88_bp[ni][mi]);
                const float *c = p88_ptab[(int)(hue * P88_HSPAN + 4096.0f) & 1023];
                cr = c[0] * bri; cg = c[1] * bri; cb = c[2] * bri;
            } else {
                k = (int)((xx + yy) * 2.0f) & 1023;
                cr = p88_gnd[k][0]; cg = p88_gnd[k][1]; cb = p88_gnd[k][2];
            }
            if (fabsf(d8 - Rs) < 1.6f) { cr = 1.0f; cg = 0.95f; cb = 0.75f; }

            ir = (int)(cr * 255.0f); if (ir > 255) ir = 255; if (ir < 0) ir = 0;
            ig = (int)(cg * 255.0f); if (ig > 255) ig = 255; if (ig < 0) ig = 0;
            ib = (int)(cb * 255.0f); if (ib > 255) ib = 255; if (ib < 0) ib = 0;
            row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                   | ((uint32_t)ig << 8) | (uint32_t)ib;
        }
    }
}
