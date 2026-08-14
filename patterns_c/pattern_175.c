/* 175 Glenz Vectors — the Amiga transparent-polygon trick, two solids deep.
 * A cube and an icosahedron are drawn interpenetrating with every face filled
 * additively and nothing culled, so the picture is the sum of all 26 faces:
 * where two faces overlap the colour doubles, where four overlap it blazes,
 * and the solids read as blown glass whose internal structure is visible.
 * Faces are scan-converted with fractional end-of-span coverage so the edges
 * stay soft, and each face's outline is laid over its fill a little brighter.
 * The two solids counter-rotate about different axes; away from the glass the
 * frame is black. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p175_up;

#define P175_W 480
#define P175_H 360

static float p175_acc[P175_W * P175_H * 3];
static unsigned char p175_img[P175_W * P175_H * 3];
static unsigned char p175_tone[1024];
static float p175_lo[P175_H], p175_hi[P175_H];
static int p175_stamp[P175_H];
static int p175_gen;
static int *p175_xm;
static int p175_xmw;
static int p175_ready;
static uint32_t p175_seedc;
static float p175_col[64][3];
static float p175_hue0, p175_huew, p175_sp1, p175_sp2, p175_mix;

static const signed char p175_cubef[6][4] = {
    {0, 1, 3, 2}, {4, 6, 7, 5}, {0, 4, 5, 1},
    {2, 3, 7, 6}, {0, 2, 6, 4}, {1, 5, 7, 3}
};
static const signed char p175_icof[20][3] = {
    { 0,11, 5}, { 0, 5, 1}, { 0, 1, 7}, { 0, 7,10}, { 0,10,11},
    { 1, 5, 9}, { 5,11, 4}, {11,10, 2}, {10, 7, 6}, { 7, 1, 8},
    { 3, 9, 4}, { 3, 4, 2}, { 3, 2, 6}, { 3, 6, 8}, { 3, 8, 9},
    { 4, 9, 5}, { 2, 4,11}, { 6, 2,10}, { 8, 6, 7}, { 9, 8, 1}
};
static float p175_cubev[8][3], p175_icov[12][3];

static uint32_t p175_rs;
static float p175_rf(void)
{
    p175_rs ^= p175_rs << 13; p175_rs ^= p175_rs >> 17; p175_rs ^= p175_rs << 5;
    return (float)(p175_rs >> 8) * (1.0f / 16777216.0f);
}

static void p175_setup(uint32_t seed)
{
    int i;
    p175_rs = seed ? seed ^ 0x61E42C05u : 0x61E42C05u;
    p175_rf(); p175_rf();
    p175_hue0 = p175_rf();
    p175_huew = 0.18f + p175_rf() * 0.60f;
    p175_sp1  = (p175_rf() < 0.5f ? -1.0f : 1.0f) * (0.0022f + p175_rf() * 0.0022f);
    p175_sp2  = (p175_rf() < 0.5f ? -1.0f : 1.0f) * (0.0018f + p175_rf() * 0.0022f);
    p175_mix  = 0.80f + p175_rf() * 0.45f;
    if (!p175_ready) {
        static const float PH = 1.61803398875f;
        float n = sqrtf(1.0f + PH * PH), inv3 = 1.0f / 1.7320508f;
        float vv[12][3] = {
            {-1.0f, PH, 0.0f}, { 1.0f, PH, 0.0f}, {-1.0f,-PH, 0.0f}, { 1.0f,-PH, 0.0f},
            { 0.0f,-1.0f, PH}, { 0.0f, 1.0f, PH}, { 0.0f,-1.0f,-PH}, { 0.0f, 1.0f,-PH},
            { PH, 0.0f,-1.0f}, { PH, 0.0f, 1.0f}, {-PH, 0.0f,-1.0f}, {-PH, 0.0f, 1.0f}
        };
        for (i = 0; i < 8; i++) {
            p175_cubev[i][0] = ((i & 1) ? 1.0f : -1.0f) * inv3;
            p175_cubev[i][1] = ((i & 2) ? 1.0f : -1.0f) * inv3;
            p175_cubev[i][2] = ((i & 4) ? 1.0f : -1.0f) * inv3;
        }
        for (i = 0; i < 12; i++) {
            p175_icov[i][0] = vv[i][0] / n;
            p175_icov[i][1] = vv[i][1] / n;
            p175_icov[i][2] = vv[i][2] / n;
        }
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (3.6f / 1024.0f)));
            p175_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p175_ready = 1;
    }
    p175_seedc = seed;
}

static void p175_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 64; i++) {
        float hue = p175_hue0 + p175_huew * ((float)i / 63.0f);
        float r, g, b, mx;
        uint32_t p;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p175_col[i][0] = 0.10f + 0.90f * r / mx;
        p175_col[i][1] = 0.10f + 0.90f * g / mx;
        p175_col[i][2] = 0.10f + 0.90f * b / mx;
    }
}

static void p175_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= P175_W - 1 || yi >= P175_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p175_acc + (yi * P175_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P175_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p175_line(float x0, float y0, float x1, float y1,
                      const float *c, float w)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n, i;
    if (len < 0.001f || len > 1200.0f) return;
    n = (int)len + 1;
    dx /= (float)n; dy /= (float)n;
    for (i = 0; i <= n; i++)
        p175_splat(x0 + dx * (float)i, y0 + dy * (float)i, c, w);
}

/* additive convex-polygon fill with fractional coverage at both span ends */
static void p175_face(const float *vx, const float *vy, int n,
                      const float *c, float w)
{
    int i, y, ylo = P175_H, yhi = -1;
    ++p175_gen;
    for (i = 0; i < n; i++) {
        int a = i, b = (i + 1) % n;
        float ax = vx[a], ay = vy[a], bx = vx[b], by = vy[b], t, dxdy;
        int y0, y1;
        if (ay == by) continue;
        if (ay > by) { t = ax; ax = bx; bx = t; t = ay; ay = by; by = t; }
        dxdy = (bx - ax) / (by - ay);
        y0 = (int)ceilf(ay - 0.5f); y1 = (int)ceilf(by - 0.5f) - 1;
        if (y0 < 0) y0 = 0;
        if (y1 > P175_H - 1) y1 = P175_H - 1;
        for (y = y0; y <= y1; y++) {
            float x = ax + ((float)y + 0.5f - ay) * dxdy;
            if (p175_stamp[y] != p175_gen) {
                p175_stamp[y] = p175_gen;
                if (y < ylo) ylo = y;
                if (y > yhi) yhi = y;
                p175_lo[y] = x; p175_hi[y] = x;
            } else {
                if (x < p175_lo[y]) p175_lo[y] = x;
                if (x > p175_hi[y]) p175_hi[y] = x;
            }
        }
    }
    if (yhi < ylo) return;
    for (y = ylo; y <= yhi; y++) {
        float xa, xb;
        if (p175_stamp[y] != p175_gen) continue;   /* row never spanned */
        xa = p175_lo[y]; xb = p175_hi[y];
        int ia, ib, x;
        float *p;
        if (xb < 0.0f || xa > (float)(P175_W - 1) || xb <= xa) continue;
        if (xa < 0.0f) xa = 0.0f;
        if (xb > (float)(P175_W - 1)) xb = (float)(P175_W - 1);
        ia = (int)xa; ib = (int)xb;
        p = p175_acc + (y * P175_W) * 3;
        if (ia == ib) {
            float f = (xb - xa) * w;
            p[ia * 3 + 0] += c[0] * f; p[ia * 3 + 1] += c[1] * f;
            p[ia * 3 + 2] += c[2] * f;
            continue;
        }
        {
            float f = (1.0f - (xa - (float)ia)) * w;
            p[ia * 3 + 0] += c[0] * f; p[ia * 3 + 1] += c[1] * f;
            p[ia * 3 + 2] += c[2] * f;
        }
        for (x = ia + 1; x < ib; x++) {
            p[x * 3 + 0] += c[0] * w; p[x * 3 + 1] += c[1] * w;
            p[x * 3 + 2] += c[2] * w;
        }
        {
            float f = (xb - (float)ib) * w;
            p[ib * 3 + 0] += c[0] * f; p[ib * 3 + 1] += c[1] * f;
            p[ib * 3 + 2] += c[2] * f;
        }
    }
}

static void p175_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P175_W * P175_H * 3; i++) {
        int ti = (int)(p175_acc[i] * 256.0f);
        p175_img[i] = p175_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p175_xmw != w) {
        free(p175_xm);
        p175_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p175_xm[x] = (int)(((long long)x * (P175_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p175_xmw = w;
    }
    jd_up_blit(&p175_up, fb, w, h, p175_img, P175_W, P175_H);
}

static void p175_rot(const float *v, float ca, float sa, float cb, float sb,
                     float cc, float sc, float *o)
{
    float x = v[0], y = v[1], z = v[2], x1, y1, z1, x2, y2, z2;
    x1 = x * cc - y * sc; y1 = x * sc + y * cc; z1 = z;
    x2 = x1 * cb + z1 * sb; z2 = -x1 * sb + z1 * cb;
    y2 = y1 * ca - z2 * sa; z2 = y1 * sa + z2 * ca;
    o[0] = x2; o[1] = y2; o[2] = z2;
}

void pattern_175(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float cx = P175_W * 0.5f, cy = P175_H * 0.5f;
    float sc = (float)P175_H * 0.415f, D = 3.2f;
    float pc[8][3], pi[12][3], px[12], py[12], pz[12];
    int i, f, k;
    (void)sl;
    if (!p175_ready || p175_seedc != seed) p175_setup(seed);
    p175_hues(pal);
    memset(p175_acc, 0, sizeof p175_acc);

    for (k = 0; k < 2; k++) {
        float a = (k ? p175_sp2 : p175_sp1) * t;
        float b = (k ? p175_sp1 : p175_sp2) * t * 0.73f + (k ? 1.7f : 0.0f);
        float g = (k ? -p175_sp2 : p175_sp1) * t * 0.41f;
        float ca = cosf(a), sa = sinf(a), cb = cosf(b), sb = sinf(b);
        float cc = cosf(g), scc = sinf(g);
        float sz = (k ? p175_mix : 1.0f) * (1.0f + 0.05f * sinf(t * 0.00081f));
        int nv = k ? 12 : 8, nf = k ? 20 : 6, nfv = k ? 3 : 4;
        for (i = 0; i < nv; i++) {
            float o[3];
            float den;
            p175_rot(k ? p175_icov[i] : p175_cubev[i], ca, sa, cb, sb, cc, scc, o);
            o[0] *= sz; o[1] *= sz; o[2] *= sz;
            den = D - o[2];
            if (den < 0.5f) den = 0.5f;
            px[i] = cx + o[0] * sc * (D / den);
            py[i] = cy + o[1] * sc * (D / den);
            pz[i] = o[2];
            if (k) { pi[i][0] = o[0]; pi[i][1] = o[1]; pi[i][2] = o[2]; }
            else   { pc[i][0] = o[0]; pc[i][1] = o[1]; pc[i][2] = o[2]; }
        }
        for (f = 0; f < nf; f++) {
            float vx[4], vy[4], zc = 0.0f, wgt;
            const float *c;
            for (i = 0; i < nfv; i++) {
                int vi = k ? p175_icof[f][i] : p175_cubef[f][i];
                vx[i] = px[vi]; vy[i] = py[vi]; zc += pz[vi];
            }
            zc /= (float)nfv;
            c = p175_col[(int)((float)f / (float)nf * 52.0f + (k ? 10.0f : 34.0f)) & 63];
            wgt = (0.13f + 0.10f * (zc + 1.0f)) * (k ? 0.85f : 1.0f);
            p175_face(vx, vy, nfv, c, wgt);
            for (i = 0; i < nfv; i++) {
                int j = (i + 1) % nfv;
                p175_line(vx[i], vy[i], vx[j], vy[j], c, 0.30f + 0.22f * (zc + 1.0f));
            }
        }
        (void)pc; (void)pi;
    }
    p175_blit(fb, w, h);
}
