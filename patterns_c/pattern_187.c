/* 187 Harmonic Lantern — a spherical-harmonic surface hung as a wire lantern.
 * The radius is Bourke's harmonic sum
 *   r(u,v) = sin(m0 u)^n0 + cos(m1 u)^n1 + sin(m2 v)^n2 + cos(m3 v)^n3,
 * mapped to the sphere by x = r sin u cos v, y = r cos u, z = r sin u sin v, and
 * drawn only as its parametric grid: 22 latitude hoops and 28 meridians, no
 * surface, so the far wall of the lantern shows through the near one. Because r
 * separates into a u-part and a v-part, both are tabulated once per frame and
 * every one of the 15 000 vertices is then two table reads — which is what buys
 * the wire count. Two independent integer parameter sets are held at all times
 * and linearly crossfaded; when the crossfade lands, the retired set is
 * re-rolled, so the lantern morphs endlessly between lobe counts without ever
 * cutting. Depth sets brightness and hue rides |r|, so lobes swinging toward
 * the viewer light up as they come. Glowing wire on black, ~90% near-zero. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p187_up;

#define CW 480
#define CH 360
#define NU 512
#define NV 800
#define NLAT 22
#define NLON 24

static float p187_acc[CW * CH * 3];
static unsigned char p187_img[CW * CH * 3];
static unsigned char p187_tone[2048];
static int *p187_xm;
static int p187_xmw;
static float p187_fu[2][NU], p187_gv[2][NV];
static float p187_F[NU], p187_G[NV];
static int p187_m[2][4], p187_n[2][4];
static float p187_s, p187_sspd;
static float p187_hue0, p187_huew, p187_ry, p187_rx;
static uint32_t p187_seedc;
static int p187_ready, p187_tabs;

static uint32_t p187_rs;
static float p187_rf(void)
{
    p187_rs ^= p187_rs << 13; p187_rs ^= p187_rs >> 17; p187_rs ^= p187_rs << 5;
    return (float)(p187_rs >> 8) * (1.0f / 16777216.0f);
}

static void p187_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static float p187_ipow(float b, int e)
{
    float r = 1.0f;
    while (e-- > 0) r *= b;
    return r;
}

static void p187_roll(int slot)
{
    int i, j;
    for (j = 0; j < 4; j++) {
        p187_m[slot][j] = (int)(p187_rf() * 7.0f);
        p187_n[slot][j] = 1 + (int)(p187_rf() * 4.0f);
    }
    for (i = 0; i < NU; i++) {
        float u = 3.14159265f * (float)i / (float)(NU - 1);
        p187_fu[slot][i] = p187_ipow(sinf((float)p187_m[slot][0] * u), p187_n[slot][0])
                         + p187_ipow(cosf((float)p187_m[slot][1] * u), p187_n[slot][1]);
    }
    for (i = 0; i < NV; i++) {
        float v = 6.2831853f * (float)i / (float)(NV - 1);
        p187_gv[slot][i] = p187_ipow(sinf((float)p187_m[slot][2] * v), p187_n[slot][2])
                         + p187_ipow(cosf((float)p187_m[slot][3] * v), p187_n[slot][3]);
    }
}

static void p187_setup(uint32_t seed)
{
    int i;
    p187_rs = seed ? seed ^ 0x8A0F17u : 0x8A0F17u;
    p187_rf(); p187_rf();
    p187_roll(0); p187_roll(1);
    p187_s = 0.0f;
    p187_sspd = 0.00085f + p187_rf() * 0.00075f;
    p187_hue0 = p187_rf();
    p187_huew = 0.14f + p187_rf() * 0.46f;
    p187_ry = (p187_rf() < 0.5f ? -1.0f : 1.0f) * (0.0021f + p187_rf() * 0.0022f);
    p187_rx = 0.00041f + p187_rf() * 0.00043f;
    if (!p187_tabs) {
        for (i = 0; i < 2048; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (5.2f / 2048.0f)));
            p187_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p187_tabs = 1;
    }
    p187_ready = 1;
    p187_seedc = seed;
}

static void p187_splat(float x, float y, const float *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 1.0f || y < 1.0f || xi >= CW - 2 || yi >= CH - 2) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p187_acc + (yi * CW + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * wgt; w1 = fx * (1.0f - fy) * wgt;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += CW * 3;
    w0 = (1.0f - fx) * fy * wgt; w1 = fx * fy * wgt;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p187_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p187_xmw != w) {
        free(p187_xm);
        p187_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p187_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p187_xmw = w;
    }
    jd_up_blit(&p187_up, fb, w, h, p187_img, CW, CH);
}

void pattern_187(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, s, ca, sa, cb, sb, cx, cy, sc, col[48][3];
    float su[NU], cu[NU], sv[NV], cv[NV];
    int i, j, k, o;
    (void)sl;
    if (!p187_ready || p187_seedc != seed) p187_setup(seed);

    p187_s += p187_sspd;
    if (p187_s >= 1.0f) {
        memcpy(p187_fu[0], p187_fu[1], sizeof p187_fu[0]);
        memcpy(p187_gv[0], p187_gv[1], sizeof p187_gv[0]);
        p187_roll(1);
        p187_s = 0.0f;
    }
    s = p187_s; s = s * s * (3.0f - 2.0f * s);
    for (i = 0; i < NU; i++) {
        float u = 3.14159265f * (float)i / (float)(NU - 1);
        p187_F[i] = p187_fu[0][i] + (p187_fu[1][i] - p187_fu[0][i]) * s;
        su[i] = sinf(u); cu[i] = cosf(u);
    }
    for (i = 0; i < NV; i++) {
        float v = 6.2831853f * (float)i / (float)(NV - 1);
        p187_G[i] = p187_gv[0][i] + (p187_gv[1][i] - p187_gv[0][i]) * s;
        sv[i] = sinf(v); cv[i] = cosf(v);
    }
    for (k = 0; k < 48; k++)
        p187_pal3(pal, p187_hue0 + p187_huew * ((float)k * (1.0f / 48.0f)), 0.88f, col[k]);

    ca = cosf(t * p187_ry); sa = sinf(t * p187_ry);
    { float b = 0.42f + 0.30f * sinf(t * p187_rx); cb = cosf(b); sb = sinf(b); }
    cx = CW * 0.5f; cy = CH * 0.5f; sc = 82.0f;

    memset(p187_acc, 0, sizeof p187_acc);
    for (k = 0; k < NLAT + NLON; k++) {
        int lat = k < NLAT;
        int ns = lat ? NV : NU;
        for (j = 0; j < ns; j++) {
            int iu, iv;
            float r, x, y, z, xr, zr, yr, zz, pd, ip, px, py, dep, v, hue;
            const float *c;
            if (lat) { iu = (k + 1) * (NU - 1) / (NLAT + 1); iv = j; }
            else     { iu = j; iv = k - NLAT; iv = iv * (NV - 1) / NLON; }
            r = p187_F[iu] + p187_G[iv];
            x = r * su[iu] * cv[iv];
            y = r * cu[iu];
            z = r * su[iu] * sv[iv];
            xr = x * ca - z * sa; zr = x * sa + z * ca;
            yr = y * cb - zr * sb; zz = y * sb + zr * cb;
            pd = 9.2f - zz;
            if (pd < 0.6f) continue;
            ip = 8.6f / pd;
            px = cx + xr * ip * sc; py = cy + yr * ip * sc;
            dep = (zz + 3.2f) * 0.16f;
            if (dep < 0.0f) dep = 0.0f; else if (dep > 1.0f) dep = 1.0f;
            hue = fabsf(r) * 0.30f + 0.10f * (float)iu * (1.0f / NU);
            if (hue > 1.0f) hue = 1.0f;
            c = col[(int)(hue * 47.0f)];
            v = (0.14f + 0.86f * dep * dep) * (lat ? 0.62f : 0.52f);
            { float rr = fabsf(r) * 3.4f; if (rr < 1.0f) v *= rr; }
            p187_splat(px, py, c, v * 0.22f);
        }
    }
    for (o = 0; o < CW * CH * 3; o++) {
        int v = (int)(p187_acc[o] * 1500.0f);
        p187_img[o] = p187_tone[v < 0 ? 0 : v > 2047 ? 2047 : v];
    }
    p187_blit(fb, w, h);
}
