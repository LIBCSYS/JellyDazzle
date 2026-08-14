/* 156 Apollonian Tide — a true Apollonian gasket breathing under an inversion.
 * The packing is generated from Descartes' circle theorem in its linear form:
 * for four mutually tangent circles the two solutions satisfy
 *   (k4, k4 z4) = 2 (k1+k2+k3, k1 z1 + k2 z2 + k3 z3) - (k0, k0 z0),
 * so the whole gasket falls out of one subtraction per new circle, recursed
 * until the radius drops below a threshold (~1500 circles). Every frame the
 * packing is pushed through a circle inversion whose centre orbits just
 * outside the rim — inversion maps circles to circles exactly, so the gasket
 * stays a gasket while its scale distribution slides: detail swells on one
 * side, packs tight on the other, a slow tide. Rings only, black interiors;
 * hue tracks log radius so scale reads as colour. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p156_up;

#define CW 480
#define CH 360
#define MAXC 2400
#define STKN 3000

static float p156_acc[CW * CH * 3];
static float p156_tmp[CW * CH * 3];
static unsigned char p156_img[CW * CH * 3];
static unsigned char p156_tone[1024];
static int *p156_xm;
static int p156_xmw;
static float p156_cx[MAXC], p156_cy[MAXC], p156_cr[MAXC], p156_cl[MAXC];
static int p156_nc;
static float p156_hue[48][3];
static float p156_hue0, p156_huew, p156_pd, p156_prho, p156_pspd;
static uint32_t p156_seedc;
static int p156_ready;

typedef struct { float k[4], x[4], y[4]; } p156_quad;
static p156_quad p156_stk[STKN];

static uint32_t p156_rs;
static float p156_rf(void)
{
    p156_rs ^= p156_rs << 13; p156_rs ^= p156_rs >> 17; p156_rs ^= p156_rs << 5;
    return (float)(p156_rs >> 8) * (1.0f / 16777216.0f);
}

static void p156_add(float k, float kx, float ky)
{
    float r;
    if (p156_nc >= MAXC || fabsf(k) < 1e-6f) return;
    r = 1.0f / fabsf(k);
    p156_cx[p156_nc] = kx / k;
    p156_cy[p156_nc] = ky / k;
    p156_cr[p156_nc] = r;
    p156_cl[p156_nc] = logf(r);
    p156_nc++;
}

static void p156_build(uint32_t seed)
{
    float rmin, th0, kc, rc;
    int sp = 0, i;
    p156_rs = seed ? seed ^ 0xA9010Fu : 0xA9010Fu;
    p156_rf(); p156_rf();
    th0 = p156_rf() * 6.2831853f;
    rmin = 0.0075f + p156_rf() * 0.0065f;
    p156_hue0 = p156_rf();
    p156_huew = 0.07f + p156_rf() * 0.38f;
    p156_pd = 2.10f + p156_rf() * 1.10f;
    p156_prho = 3.20f + p156_rf() * 2.60f;
    p156_pspd = (p156_rf() < 0.5f ? -1.0f : 1.0f) * (0.00135f + p156_rf() * 0.00095f);

    kc = 1.0f / (2.0f * 1.7320508f - 3.0f);      /* 2.1547 */
    rc = 1.0f - 1.0f / kc;
    p156_nc = 0;
    p156_add(-1.0f, 0.0f, 0.0f);
    {
        p156_quad q;
        q.k[0] = -1.0f; q.x[0] = 0.0f; q.y[0] = 0.0f;
        for (i = 0; i < 3; i++) {
            float a = th0 + (float)i * 2.0943951f;
            float cx = rc * cosf(a), cy = rc * sinf(a);
            p156_add(kc, kc * cx, kc * cy);
            q.k[i + 1] = kc; q.x[i + 1] = kc * cx; q.y[i + 1] = kc * cy;
        }
        p156_stk[sp++] = q;
    }
    while (sp > 0 && p156_nc < MAXC) {
        p156_quad q = p156_stk[--sp];
        int tri[3][3] = {{1, 2, 3}, {0, 2, 3}, {0, 1, 3}};
        int oth[3] = {0, 1, 2};
        for (i = 0; i < 3; i++) {
            int a = tri[i][0], b = tri[i][1], c = tri[i][2], o = oth[i];
            float nk = 2.0f * (q.k[a] + q.k[b] + q.k[c]) - q.k[o];
            float nx = 2.0f * (q.x[a] + q.x[b] + q.x[c]) - q.x[o];
            float ny = 2.0f * (q.y[a] + q.y[b] + q.y[c]) - q.y[o];
            float r;
            if (fabsf(nk) < 1e-5f) continue;
            r = 1.0f / fabsf(nk);
            if (r < rmin || r > 1.2f) continue;
            p156_add(nk, nx, ny);
            if (sp < STKN - 1 && p156_nc < MAXC) {
                p156_quad n;
                n.k[0] = q.k[a]; n.x[0] = q.x[a]; n.y[0] = q.y[a];
                n.k[1] = q.k[b]; n.x[1] = q.x[b]; n.y[1] = q.y[b];
                n.k[2] = q.k[c]; n.x[2] = q.x[c]; n.y[2] = q.y[c];
                n.k[3] = nk;     n.x[3] = nx;     n.y[3] = ny;
                p156_stk[sp++] = n;
            }
        }
    }
    if (!p156_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (8.5f / 1024.0f)));
            p156_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p156_ready = 1;
    }
    p156_seedc = seed;
}

static void p156_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 48; i++) {
        float hue = p156_hue0 + p156_huew * ((float)i / 47.0f);
        uint32_t p; float r, g, b, mx;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p156_hue[i][0] = 0.10f + 0.90f * r / mx;
        p156_hue[i][1] = 0.10f + 0.90f * g / mx;
        p156_hue[i][2] = 0.10f + 0.90f * b / mx;
    }
}

static void p156_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= CW - 1 || yi >= CH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p156_acc + (yi * CW + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += CW * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p156_blur(void)
{
    int y, x, c;
    for (y = 1; y < CH - 1; y++)
        for (x = 1; x < CW - 1; x++) {
            int o = (y * CW + x) * 3;
            for (c = 0; c < 3; c++)
                p156_tmp[o + c] = p156_acc[o + c] * 0.50f
                    + 0.125f * (p156_acc[o + c - 3] + p156_acc[o + c + 3]
                              + p156_acc[o + c - CW * 3] + p156_acc[o + c + CW * 3]);
        }
    for (y = 1; y < CH - 1; y++)
        memcpy(p156_acc + (y * CW + 1) * 3, p156_tmp + (y * CW + 1) * 3,
               sizeof(float) * 3 * (CW - 2));
}

static void p156_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < CW * CH * 3; i++) {
        int ti = (int)(p156_acc[i] * 256.0f);
        p156_img[i] = p156_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p156_xmw != w) {
        free(p156_xm);
        p156_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p156_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p156_xmw = w;
    }
    jd_up_blit(&p156_up, fb, w, h, p156_img, CW, CH);
}

void pattern_156(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, Px, Py, rho2, ox, oy, sc, ca, sa;
    int i, j;
    (void)sl;
    if (!p156_ready || p156_seedc != seed) p156_build(seed);
    p156_hues(pal);
    memset(p156_acc, 0, sizeof p156_acc);

    {
        float a = t * p156_pspd;
        float d = p156_pd * (1.0f + 0.10f * sinf(t * 0.00039f));
        Px = d * cosf(a); Py = d * sinf(a);
        rho2 = p156_prho * (1.0f + 0.08f * sinf(t * 0.00051f + 2.0f));
    }
    {   /* frame on the image of the rim circle so the picture stays centred */
        float dx = 0.0f - Px, dy = 0.0f - Py;
        float den = dx * dx + dy * dy - 1.0f;
        float s;
        if (fabsf(den) < 0.05f) den = den < 0.0f ? -0.05f : 0.05f;
        s = rho2 / den;
        ox = Px + s * dx; oy = Py + s * dy;
        sc = (float)CH * 0.470f / (fabsf(s) > 1e-4f ? fabsf(s) : 1e-4f);
    }
    ca = cosf(t * 0.00068f); sa = sinf(t * 0.00068f);

    for (i = 0; i < p156_nc; i++) {
        float dx = p156_cx[i] - Px, dy = p156_cy[i] - Py;
        float den = dx * dx + dy * dy - p156_cr[i] * p156_cr[i];
        float s, gx, gy, gr, rpx, wgt, ph;
        const float *col;
        int ns;
        if (fabsf(den) < 0.012f) continue;
        s = rho2 / den;
        gx = Px + s * dx; gy = Py + s * dy;
        gr = fabsf(s) * p156_cr[i];
        /* to screen */
        {
            float ux = (gx - ox) * sc, uy = (gy - oy) * sc;
            gx = CW * 0.5f + ux * ca - uy * sa;
            gy = CH * 0.5f + ux * sa + uy * ca;
            rpx = gr * sc;
        }
        if (rpx < 0.55f || rpx > 900.0f) continue;
        if (gx + rpx < 0.0f || gx - rpx > CW || gy + rpx < 0.0f || gy - rpx > CH) continue;
        ph = p156_cl[i] * 2.1f - t * 0.016f;
        wgt = 0.46f + 0.94f * (0.5f + 0.5f * sinf(ph));
        {
            float hi = (p156_cl[i] + 5.2f) * (47.0f / 5.2f);
            int hx = (int)(hi < 0.0f ? 0.0f : hi > 47.0f ? 47.0f : hi);
            col = p156_hue[hx];
        }
        ns = (int)(rpx * 7.0f) + 8;
        if (ns > 900) ns = 900;
        {
            float step = 6.2831853f / (float)ns;
            float wpt = wgt * (0.85f + 0.6f * (rpx / (rpx + 26.0f)));
            wpt *= (6.2831853f * rpx / (float)ns) * 0.85f + 0.14f;
            for (j = 0; j < ns; j++) {
                float a = (float)j * step;
                p156_splat(gx + rpx * cosf(a), gy + rpx * sinf(a), col, wpt);
            }
        }
    }
    p156_blur();
    p156_blit(fb, w, h);
}
