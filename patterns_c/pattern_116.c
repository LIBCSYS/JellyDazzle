/* 116 Hilbert Unfold — a space-filling curve caught mid-construction.
 * The order-6 Hilbert curve (4096 cells) is precomputed once, and so is the
 * position of every one of its points at each coarser order 1..6 — the centre
 * of the level-n cell that contains it. Each frame the drawn point is a chain
 * of lerps up that hierarchy, Q_n = lerp(Q_{n-1}, pos_n, s_n), with an
 * independent slow sine driving each level's s_n. Levels therefore fold in and
 * out on their own clocks: the curve collapses toward a fat order-2 zigzag,
 * grows its order-5 detail back, ripples, and unfolds again, always as one
 * continuous ribbon that never crosses itself. Hue runs along the arc length
 * and a light pulse chases the path. Ribbon on black: an overlay.
 */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
static jd_up p116_up;

#define P116_LW 640
#define P116_LH 480
#define P116_ORD 6
#define P116_NP  4096                      /* 4^ORD */

static float p116_px[P116_ORD + 1][P116_NP];
static float p116_py[P116_ORD + 1][P116_NP];
static int   p116_built;
static float p116_acc[P116_LW * P116_LH * 3];
static float p116_tmp[P116_LW * P116_LH * 3];
static unsigned char p116_img[P116_LW * P116_LH * 3];
static float p116_ramp[256][3];

static void p116_ramp_build(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i * 128) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 8.0f) mx = 8.0f;
        p116_ramp[i][0] = r / mx; p116_ramp[i][1] = g / mx; p116_ramp[i][2] = b / mx;
    }
}

/* Hilbert index -> (x,y) on an n x n grid, the standard bit-rotation form */
static void p116_d2xy(int n, int d, int *rx_, int *ry_)
{
    int rx, ry, s, t = d, x = 0, y = 0;
    for (s = 1; s < n; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        if (ry == 0) {
            int tmp;
            if (rx == 1) { x = s - 1 - x; y = s - 1 - y; }
            tmp = x; x = y; y = tmp;
        }
        x += s * rx; y += s * ry;
        t /= 4;
    }
    *rx_ = x; *ry_ = y;
}

static void p116_build(void)
{
    int lvl, k;
    for (lvl = 1; lvl <= P116_ORD; lvl++) {
        int side = 1 << lvl;
        int shift = 2 * (P116_ORD - lvl);
        float inv = 1.0f / (float)side;
        for (k = 0; k < P116_NP; k++) {
            int x, y;
            p116_d2xy(side, k >> shift, &x, &y);
            p116_px[lvl][k] = ((float)x + 0.5f) * inv - 0.5f;
            p116_py[lvl][k] = ((float)y + 0.5f) * inv - 0.5f;
        }
    }
    p116_built = 1;
}

static void p116_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1, w2, w3; float *p;
    if (xi < 0 || yi < 0 || xi >= P116_LW - 1 || yi >= P116_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    w2 = (1.0f - fx) * fy * w;         w3 = fx * fy * w;
    p = p116_acc + ((size_t)yi * P116_LW + xi) * 3;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P116_LW * 3;
    p[0] += c[0] * w2; p[1] += c[1] * w2; p[2] += c[2] * w2;
    p[3] += c[0] * w3; p[4] += c[1] * w3; p[5] += c[2] * w3;
}

static void p116_seg(float x0, float y0, float x1, float y1,
                     const float *col, float w)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n, i;
    if (len > 400.0f) return;
    n = (int)(len * 1.15f) + 1;
    {
        float ix = dx / (float)n, iy = dy / (float)n;
        for (i = 0; i < n; i++)
            p116_splat(x0 + ix * (float)i, y0 + iy * (float)i, col, w);
    }
}

static void p116_blit(uint32_t *fb, int w, int h)
{
    int i, x, y, c, n = P116_LW * P116_LH * 3;
    for (y = 0; y < P116_LH; y++) {
        const float *s = p116_acc + (size_t)y * P116_LW * 3;
        float *d = p116_tmp + (size_t)y * P116_LW * 3;
        for (x = 0; x < P116_LW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < P116_LW - 1 ? x + 1 : P116_LW - 1;
            for (c = 0; c < 3; c++)
                d[x * 3 + c] = 0.27f * (s[xm * 3 + c] + s[xp * 3 + c]) +
                               0.46f * s[x * 3 + c];
        }
    }
    for (x = 0; x < P116_LW; x++)
        for (y = 0; y < P116_LH; y++) {
            int ym = y > 0 ? y - 1 : 0, yp = y < P116_LH - 1 ? y + 1 : P116_LH - 1;
            for (c = 0; c < 3; c++) {
                size_t o = (size_t)x * 3 + (size_t)c;
                float v = 0.27f * (p116_tmp[(size_t)ym * P116_LW * 3 + o] +
                                   p116_tmp[(size_t)yp * P116_LW * 3 + o]) +
                          0.46f * p116_tmp[(size_t)y * P116_LW * 3 + o];
                p116_acc[(size_t)y * P116_LW * 3 + o] += 1.05f * v;
            }
        }
    for (i = 0; i < n; i++) {
        float cc = p116_acc[i], v = 255.0f * cc / (0.9f + cc);
        p116_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    jd_up_blit(&p116_up, fb, w, h, p116_img, P116_LW, P116_LH);
}

void pattern_116(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sp = (float)(seed & 1023) * 0.006136f;
    float s[P116_ORD + 1], ca, sa, S, cx, cy, pulse;
    float qx = 0.0f, qy = 0.0f, lx = 0.0f, ly = 0.0f;
    int k, lvl, hbase;
    (void)sl;

    if (!p116_built) p116_build();
    p116_ramp_build(pal);
    memset(p116_acc, 0, sizeof p116_acc);

    for (lvl = 2; lvl <= P116_ORD; lvl++) {
        float fr = (float)lvl;
        s[lvl] = 0.5f + 0.5f * sinf((0.00031f + 0.00019f * fr) * t
                                    + fr * 1.37f + sp);
    }
    s[1] = 1.0f;
    {
        float rot = 0.00036f * t + sp;
        ca = cosf(rot); sa = sinf(rot);
    }
    S  = (float)P116_LH * (0.92f + 0.05f * sinf(0.00053f * t));
    cx = (float)P116_LW * 0.5f;
    cy = (float)P116_LH * 0.5f;
    pulse = 0.0037f * t;
    hbase = (int)(t * 0.041f + sp * 30.0f);

    for (k = 0; k < P116_NP; k++) {
        float X = p116_px[1][k], Y = p116_py[1][k];
        float ux, uy, bw;
        const float *col;
        int hi;
        for (lvl = 2; lvl <= P116_ORD; lvl++) {
            float a = s[lvl];
            X += (p116_px[lvl][k] - X) * a;
            Y += (p116_py[lvl][k] - Y) * a;
        }
        ux = cx + (X * ca - Y * sa) * S;
        uy = cy + (X * sa + Y * ca) * S * 0.98f;
        if (k) {
            float f = (float)k * (1.0f / (float)P116_NP);
            hi = (hbase + (int)(f * 210.0f)) & 255;
            col = p116_ramp[hi];
            bw = 0.42f + 0.58f * (0.5f + 0.5f * sinf(f * 25.1f - pulse * 6.2831853f));
            p116_seg(lx, ly, ux, uy, col, bw * 1.05f);
        }
        lx = ux; ly = uy;
        qx = X; qy = Y;
    }
    (void)qx; (void)qy;
    p116_blit(fb, w, h);
}
