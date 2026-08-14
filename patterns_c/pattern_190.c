/* 190 Glenz Vectors — the Amiga demo trick that predates alpha blending. Two
 * dual solids, a cube and an octahedron, counter-rotate through each other on
 * detuned axes; every face of both is filled ADDITIVELY, no z-buffer, no
 * sorting, no hidden-surface removal at all. That is the whole effect: because
 * light adds, the far faces show through the near ones and every region of
 * overlap becomes its own brighter colour, so a six-face cube reads as dozens
 * of stained-glass cells whose boundaries are the silhouettes of faces you
 * cannot otherwise see. Face alpha follows |n.z| after rotation, so a face
 * turning edge-on fades out instead of collapsing to a hard line, and the
 * twenty-four edges are stroked over the top as filament highlights. Convex
 * spans are scanline-filled with fractional end-pixel coverage, so the
 * silhouettes stay smooth while the solids turn. One glowing jewel on black,
 * ~75% of the frame at zero. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p190_up;

#define CW 480
#define CH 360

static float p190_acc[CW * CH * 3];
static unsigned char p190_img[CW * CH * 3];
static unsigned char p190_tone[2048];
static int *p190_xm;
static int p190_xmw;
static float p190_hue0, p190_huew, p190_ra[3], p190_rb[3];
static uint32_t p190_seedc;
static int p190_ready, p190_tabs;

static const float p190_cv[8][3] = {
    {-1,-1,-1},{ 1,-1,-1},{ 1, 1,-1},{-1, 1,-1},
    {-1,-1, 1},{ 1,-1, 1},{ 1, 1, 1},{-1, 1, 1}
};
static const int p190_cf[6][4] = {
    {0,1,2,3},{5,4,7,6},{4,0,3,7},{1,5,6,2},{4,5,1,0},{3,2,6,7}
};
static const int p190_ce[12][2] = {
    {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
};
static const float p190_ov[6][3] = {
    { 1,0,0},{-1,0,0},{0, 1,0},{0,-1,0},{0,0, 1},{0,0,-1}
};
static const int p190_of[8][3] = {
    {0,2,4},{2,1,4},{1,3,4},{3,0,4},{2,0,5},{1,2,5},{3,1,5},{0,3,5}
};
static const int p190_oe[12][2] = {
    {0,2},{2,1},{1,3},{3,0},{0,4},{2,4},{1,4},{3,4},{0,5},{2,5},{1,5},{3,5}
};

static uint32_t p190_rs;
static float p190_rf(void)
{
    p190_rs ^= p190_rs << 13; p190_rs ^= p190_rs >> 17; p190_rs ^= p190_rs << 5;
    return (float)(p190_rs >> 8) * (1.0f / 16777216.0f);
}

static void p190_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static void p190_setup(uint32_t seed)
{
    int i;
    p190_rs = seed ? seed ^ 0x91E42Bu : 0x91E42Bu;
    p190_rf(); p190_rf();
    p190_hue0 = p190_rf();
    p190_huew = 0.30f + p190_rf() * 0.66f;
    for (i = 0; i < 3; i++) {
        p190_ra[i] = (p190_rf() - 0.5f) * 0.0062f;
        p190_rb[i] = (p190_rf() - 0.5f) * 0.0062f;
    }
    p190_ra[1] += 0.0022f; p190_rb[0] -= 0.0019f;
    if (!p190_tabs) {
        for (i = 0; i < 2048; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (5.0f / 2048.0f)));
            p190_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p190_tabs = 1;
    }
    p190_ready = 1;
    p190_seedc = seed;
}

static void p190_rot(const float *a, float *m)
{
    float ca = cosf(a[0]), sa = sinf(a[0]);
    float cb = cosf(a[1]), sb = sinf(a[1]);
    float cc = cosf(a[2]), sc = sinf(a[2]);
    m[0] = cc * cb;
    m[1] = cc * sb * sa - sc * ca;
    m[2] = cc * sb * ca + sc * sa;
    m[3] = sc * cb;
    m[4] = sc * sb * sa + cc * ca;
    m[5] = sc * sb * ca - cc * sa;
    m[6] = -sb;
    m[7] = cb * sa;
    m[8] = cb * ca;
}

static void p190_span(int y, float xl, float xr, const float *c, float a)
{
    int x0, x1, x;
    float *p;
    if (xr <= xl) return;
    if (xl < 1.0f) xl = 1.0f;
    if (xr > (float)(CW - 2)) xr = (float)(CW - 2);
    if (xr <= xl) return;
    x0 = (int)xl; x1 = (int)xr;
    p = p190_acc + (y * CW) * 3;
    if (x0 == x1) {
        float wq = (xr - xl) * a;
        p[x0 * 3] += c[0] * wq; p[x0 * 3 + 1] += c[1] * wq; p[x0 * 3 + 2] += c[2] * wq;
        return;
    }
    {
        float wq = ((float)(x0 + 1) - xl) * a;
        p[x0 * 3] += c[0] * wq; p[x0 * 3 + 1] += c[1] * wq; p[x0 * 3 + 2] += c[2] * wq;
    }
    for (x = x0 + 1; x < x1; x++) {
        p[x * 3] += c[0] * a; p[x * 3 + 1] += c[1] * a; p[x * 3 + 2] += c[2] * a;
    }
    {
        float wq = (xr - (float)x1) * a;
        p[x1 * 3] += c[0] * wq; p[x1 * 3 + 1] += c[1] * wq; p[x1 * 3 + 2] += c[2] * wq;
    }
}

static void p190_poly(const float *px, const float *py, int n, const float *c, float a)
{
    float ylo = 1e9f, yhi = -1e9f;
    int i, y, y0, y1;
    for (i = 0; i < n; i++) {
        if (py[i] < ylo) ylo = py[i];
        if (py[i] > yhi) yhi = py[i];
    }
    y0 = (int)ylo; y1 = (int)yhi + 1;
    if (y0 < 1) y0 = 1;
    if (y1 > CH - 2) y1 = CH - 2;
    for (y = y0; y <= y1; y++) {
        float yc = (float)y + 0.5f, xl = 1e9f, xr = -1e9f;
        for (i = 0; i < n; i++) {
            int j = (i + 1 == n) ? 0 : i + 1;
            float ya = py[i], yb = py[j], x;
            if ((ya <= yc && yb > yc) || (yb <= yc && ya > yc)) {
                x = px[i] + (px[j] - px[i]) * (yc - ya) / (yb - ya);
                if (x < xl) xl = x;
                if (x > xr) xr = x;
            }
        }
        if (xr > xl) p190_span(y, xl, xr, c, a);
    }
}

static void p190_splat(float x, float y, const float *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 1.0f || y < 1.0f || xi >= CW - 2 || yi >= CH - 2) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p190_acc + (yi * CW + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * wgt; w1 = fx * (1.0f - fy) * wgt;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += CW * 3;
    w0 = (1.0f - fx) * fy * wgt; w1 = fx * fy * wgt;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p190_edge(const float *a, const float *b, const float *c, float w)
{
    float dx = b[0] - a[0], dy = b[1] - a[1];
    float len = sqrtf(dx * dx + dy * dy);
    int n = (int)(len * 1.25f) + 2, i;
    for (i = 0; i <= n; i++) {
        float s = (float)i / (float)n;
        p190_splat(a[0] + dx * s, a[1] + dy * s, c, w);
    }
}

static void p190_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p190_xmw != w) {
        free(p190_xm);
        p190_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p190_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p190_xmw = w;
    }
    jd_up_blit(&p190_up, fb, w, h, p190_img, CW, CH);
}

void pattern_190(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, ma[9], mb[9], ang[3], col[16][3];
    float sx[8], sy[8], sz[8], ox[6], oy[6], oz[6];
    float cx, cy, sc, breath;
    int i, j, k, o;
    (void)sl;
    if (!p190_ready || p190_seedc != seed) p190_setup(seed);

    for (k = 0; k < 16; k++)
        p190_pal3(pal, p190_hue0 + p190_huew * ((float)k * (1.0f / 16.0f)), 0.95f, col[k]);
    for (i = 0; i < 3; i++) ang[i] = t * p190_ra[i];
    p190_rot(ang, ma);
    for (i = 0; i < 3; i++) ang[i] = t * p190_rb[i] + 1.3f;
    p190_rot(ang, mb);

    cx = CW * 0.5f; cy = CH * 0.5f;
    breath = 1.0f + 0.10f * sinf(t * 0.00051f);
    sc = 108.0f * breath;
    memset(p190_acc, 0, sizeof p190_acc);

    for (i = 0; i < 8; i++) {
        float x = p190_cv[i][0] * 0.80f, y = p190_cv[i][1] * 0.80f, z = p190_cv[i][2] * 0.80f;
        float xr = ma[0]*x + ma[1]*y + ma[2]*z;
        float yr = ma[3]*x + ma[4]*y + ma[5]*z;
        float zr = ma[6]*x + ma[7]*y + ma[8]*z;
        float ip = 4.6f / (4.6f - zr);
        sx[i] = cx + xr * ip * sc; sy[i] = cy + yr * ip * sc; sz[i] = zr;
    }
    for (i = 0; i < 6; i++) {
        float x = p190_ov[i][0] * 1.16f, y = p190_ov[i][1] * 1.16f, z = p190_ov[i][2] * 1.16f;
        float xr = mb[0]*x + mb[1]*y + mb[2]*z;
        float yr = mb[3]*x + mb[4]*y + mb[5]*z;
        float zr = mb[6]*x + mb[7]*y + mb[8]*z;
        float ip = 4.6f / (4.6f - zr);
        ox[i] = cx + xr * ip * sc; oy[i] = cy + yr * ip * sc; oz[i] = zr;
    }

    for (k = 0; k < 6; k++) {
        float px[4], py[4], ar, a;
        for (j = 0; j < 4; j++) { px[j] = sx[p190_cf[k][j]]; py[j] = sy[p190_cf[k][j]]; }
        ar = (px[1]-px[0])*(py[2]-py[0]) - (px[2]-px[0])*(py[1]-py[0]);
        a = fabsf(ar) / (sc * sc * 1.9f);
        if (a > 1.0f) a = 1.0f;
        a = 0.055f + 0.185f * a;
        p190_poly(px, py, 4, col[(k * 5) & 15], a);
    }
    for (k = 0; k < 8; k++) {
        float px[3], py[3], ar, a;
        for (j = 0; j < 3; j++) { px[j] = ox[p190_of[k][j]]; py[j] = oy[p190_of[k][j]]; }
        ar = (px[1]-px[0])*(py[2]-py[0]) - (px[2]-px[0])*(py[1]-py[0]);
        a = fabsf(ar) / (sc * sc * 1.1f);
        if (a > 1.0f) a = 1.0f;
        a = 0.050f + 0.170f * a;
        p190_poly(px, py, 3, col[(k * 3 + 8) & 15], a);
    }
    for (k = 0; k < 12; k++) {
        float a[2], b[2], dep;
        a[0] = sx[p190_ce[k][0]]; a[1] = sy[p190_ce[k][0]];
        b[0] = sx[p190_ce[k][1]]; b[1] = sy[p190_ce[k][1]];
        dep = 0.5f + 0.5f * (sz[p190_ce[k][0]] + sz[p190_ce[k][1]]) * 0.62f;
        p190_edge(a, b, col[(k * 7) & 15], 0.10f + 0.20f * dep);
        a[0] = ox[p190_oe[k][0]]; a[1] = oy[p190_oe[k][0]];
        b[0] = ox[p190_oe[k][1]]; b[1] = oy[p190_oe[k][1]];
        dep = 0.5f + 0.5f * (oz[p190_oe[k][0]] + oz[p190_oe[k][1]]) * 0.55f;
        p190_edge(a, b, col[(k * 5 + 4) & 15], 0.09f + 0.18f * dep);
    }

    /* additive stacks desaturate toward white; push chroma back out */
    for (o = 0; o < CW * CH * 3; o += 3) {
        int va = (int)(p190_acc[o] * 660.0f);
        int vb = (int)(p190_acc[o + 1] * 660.0f);
        int vc = (int)(p190_acc[o + 2] * 660.0f);
        float r = (float)p190_tone[va < 0 ? 0 : va > 2047 ? 2047 : va];
        float g = (float)p190_tone[vb < 0 ? 0 : vb > 2047 ? 2047 : vb];
        float b = (float)p190_tone[vc < 0 ? 0 : vc > 2047 ? 2047 : vc];
        float l = (r + g + b) * (1.0f / 3.0f);
        int ri, gi, bi;
        r = l + (r - l) * 2.05f; g = l + (g - l) * 2.05f; b = l + (b - l) * 2.05f;
        ri = (int)r; gi = (int)g; bi = (int)b;
        p190_img[o]     = (unsigned char)(ri < 0 ? 0 : ri > 255 ? 255 : ri);
        p190_img[o + 1] = (unsigned char)(gi < 0 ? 0 : gi > 255 ? 255 : gi);
        p190_img[o + 2] = (unsigned char)(bi < 0 ? 0 : bi > 255 ? 255 : bi);
    }
    p190_blit(fb, w, h);
}
