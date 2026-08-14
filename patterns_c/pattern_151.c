/* 151 Chladni Filaments — cymatic nodal lines on a black plate.
 * Superposition of six square-plate modes,
 *   f(x,y) = SUM a_k [cos(n_k pi x)cos(m_k pi y) - cos(m_k pi x)cos(n_k pi y)],
 * lit as a Lorentzian ridge 1/(1+(g f)^2) so only the ZERO SET (the sand lines)
 * glows: thin bright filaments over black, everything else dark. The mode
 * amplitudes a_k breathe on slow incommensurate sines and the plate scale
 * drifts, so the nodal web morphs continuously and never cuts. Separable in
 * x and y, so the frame is two cosine tables and a 12-term dot product.
 * Sparse (>80% near-black) — built to stack under MAX. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p151_up;

#define CW 480
#define CH 360
#define NM 6                       /* modes */
#define NO (NM * 2)                /* cosine orders in play */

static float p151_val[CW * CH];
static unsigned char p151_img[CW * CH * 3];
static float p151_cx[NO][CW], p151_cy[NO][CH];
static float p151_vig[CW * CH];
static int p151_n[NM], p151_m[NM];
static int p151_ix[NM], p151_iy[NM];      /* index of order n_k / m_k in tables */
static int p151_ord[NO], p151_nord;
static float p151_ph[NM], p151_sp[NM];
static float p151_hue0, p151_huew;
static uint32_t p151_seedc;
static int p151_ready;
static int *p151_xm;
static int p151_xmw;

static uint32_t p151_rs;
static float p151_rf(void)
{
    p151_rs ^= p151_rs << 13; p151_rs ^= p151_rs >> 17; p151_rs ^= p151_rs << 5;
    return (float)(p151_rs >> 8) * (1.0f / 16777216.0f);
}

static void p151_pal3(const uint32_t *pal, float hue, float sat, float *o)
{
    uint32_t p;
    float r, g, b, mx;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
    mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    o[0] = (1.0f - sat) + sat * r / mx;
    o[1] = (1.0f - sat) + sat * g / mx;
    o[2] = (1.0f - sat) + sat * b / mx;
}

static void p151_setup(uint32_t seed)
{
    int k, x, y;
    p151_rs = seed ? seed ^ 0x51A1D1u : 0x51A1D1u;
    p151_rf(); p151_rf();
    p151_nord = 0;
    for (k = 0; k < NM; k++) {
        int n = 1 + (int)(p151_rf() * 7.0f);         /* 1..7 */
        int m = 1 + (int)(p151_rf() * 7.0f);
        if (m == n) m = n % 7 + 1;
        p151_n[k] = n; p151_m[k] = m;
        p151_ph[k] = p151_rf() * 6.2831853f;
        p151_sp[k] = 0.0016f + p151_rf() * 0.0026f;
    }
    for (k = 0; k < NM; k++) {
        int t;
        for (t = 0; t < p151_nord && p151_ord[t] != p151_n[k]; t++) ;
        if (t == p151_nord) p151_ord[p151_nord++] = p151_n[k];
        p151_ix[k] = t;
        for (t = 0; t < p151_nord && p151_ord[t] != p151_m[k]; t++) ;
        if (t == p151_nord) p151_ord[p151_nord++] = p151_m[k];
        p151_iy[k] = t;
    }
    p151_hue0 = p151_rf();
    p151_huew = 0.05f + p151_rf() * 0.28f;           /* stark band .. wide */
    if (!p151_ready) {
        for (y = 0; y < CH; y++) {
            float dy = ((float)y - CH * 0.5f) / (CH * 0.5f);
            for (x = 0; x < CW; x++) {
                float dx = ((float)x - CW * 0.5f) / (CW * 0.5f);
                float d = dx * dx * 0.72f + dy * dy;
                float v = 1.0f - 0.55f * d * d;
                p151_vig[y * CW + x] = v < 0.0f ? 0.0f : v;
            }
        }
        p151_ready = 1;
    }
    p151_seedc = seed;
}

static void p151_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p151_xmw != w) {
        free(p151_xm);
        p151_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p151_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p151_xmw = w;
    }
    jd_up_blit(&p151_up, fb, w, h, p151_img, CW, CH);
}

void pattern_151(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float amp[NM], t = (float)frame, sc, gain;
    float colA[3], colB[3];
    int k, x, y, i;
    (void)sl;
    if (!p151_ready || p151_seedc != seed) p151_setup(seed);

    sc = 1.0f + 0.10f * sinf(t * 0.0011f) + 0.05f * sinf(t * 0.00071f + 1.1f);
    for (k = 0; k < NM; k++) {
        float a = sinf(t * p151_sp[k] + p151_ph[k]);
        amp[k] = a * fabsf(a);                      /* soft dwell near zero */
    }
    for (i = 0; i < p151_nord; i++) {
        float f = (float)p151_ord[i] * 3.14159265f * sc;
        for (x = 0; x < CW; x++)
            p151_cx[i][x] = cosf(f * (((float)x + 0.5f) * (2.0f / CW) - 1.0f));
        for (y = 0; y < CH; y++)
            p151_cy[i][y] = cosf(f * (((float)y + 0.5f) * (2.0f / CH) - 1.0f) * 0.82f);
    }

    gain = 11.0f + 3.0f * sinf(t * 0.0009f);
    for (y = 0; y < CH; y++) {
        float *vp = p151_val + y * CW;
        for (x = 0; x < CW; x++) {
            float f = 0.0f;
            for (k = 0; k < NM; k++) {
                int a = p151_ix[k], b = p151_iy[k];
                f += amp[k] * (p151_cx[a][x] * p151_cy[b][y]
                             - p151_cx[b][x] * p151_cy[a][y]);
            }
            f *= gain;
            vp[x] = 1.0f / (1.0f + f * f);
        }
    }

    p151_pal3(pal, p151_hue0, 0.95f, colA);
    p151_pal3(pal, p151_hue0 + p151_huew, 0.95f, colB);
    for (y = 0; y < CH; y++) {
        const float *vp = p151_val + y * CW;
        const float *gp = p151_vig + y * CW;
        unsigned char *op = p151_img + y * CW * 3;
        float ry = ((float)y - CH * 0.5f) / (CH * 0.5f);
        for (x = 0; x < CW; x++) {
            float b = vp[x] * gp[x];
            float rx = ((float)x - CW * 0.5f) / (CW * 0.5f);
            float u = 0.5f + 0.5f * (rx * 0.6f + ry * 0.8f);
            float core = b * b * b * 0.55f;
            int c;
            for (c = 0; c < 3; c++) {
                float v = b * (colA[c] + (colB[c] - colA[c]) * u) + core;
                v = v * 255.0f + 0.5f;
                op[x * 3 + c] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
            }
        }
    }
    p151_blit(fb, w, h);
}
