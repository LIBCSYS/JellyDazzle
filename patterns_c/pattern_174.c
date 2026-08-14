/* 174 Braid Loom — concentric torus braids, properly over and under.
 * Each ring carries m cords whose radius wobbles as sin(f*theta + phase) with
 * f = P + 1/m, so after one lap every cord has slid into its neighbour's place
 * and the braid closes on itself the way a real plait does. The cosine of the
 * same argument is used as depth, and the ring is painted in theta order with
 * the cords sorted back-to-front at every step, each one first clearing a
 * casing around itself and then laying its core down — so a cord that passes
 * behind is genuinely cut by the cord in front. Interlace, not overlap.
 * Rings counter-rotate at different rates over black. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p174_up;

#define P174_W 480
#define P174_H 360
#define P174_NR 5
#define P174_MAXM 5
#define P174_TAU 6.28318530717958647692f

static float p174_acc[P174_W * P174_H * 3];
static unsigned char p174_img[P174_W * P174_H * 3];
static unsigned char p174_tone[1024];
static int *p174_xm;
static int p174_xmw;
static int p174_ready;
static uint32_t p174_seedc;
static float p174_col[64][3];
static float p174_hue0, p174_huew;
static int p174_m[P174_NR], p174_p[P174_NR];
static float p174_rad[P174_NR], p174_amp[P174_NR], p174_spd[P174_NR];

static uint32_t p174_rs;
static float p174_rf(void)
{
    p174_rs ^= p174_rs << 13; p174_rs ^= p174_rs >> 17; p174_rs ^= p174_rs << 5;
    return (float)(p174_rs >> 8) * (1.0f / 16777216.0f);
}

static void p174_setup(uint32_t seed)
{
    int i;
    float r = 0.155f;
    p174_rs = seed ? seed ^ 0x8A21D0Fu : 0x8A21D0Fu;
    p174_rf(); p174_rf();
    p174_hue0 = p174_rf();
    p174_huew = 0.14f + p174_rf() * 0.55f;
    for (i = 0; i < P174_NR; i++) {
        p174_m[i]   = 3 + (int)(p174_rf() * 3.0f);          /* 3..5 cords     */
        p174_p[i]   = 4 + (int)(p174_rf() * 7.0f);          /* 4..10 windings */
        p174_rad[i] = r;
        p174_amp[i] = 0.019f + p174_rf() * 0.010f;
        p174_spd[i] = (i & 1 ? -1.0f : 1.0f) * (0.0022f + p174_rf() * 0.0028f);
        r += 0.062f + p174_rf() * 0.014f;
    }
    if (!p174_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (4.0f / 1024.0f)));
            p174_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p174_ready = 1;
    }
    p174_seedc = seed;
}

static void p174_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 64; i++) {
        float hue = p174_hue0 + p174_huew * ((float)i / 63.0f);
        float r, g, b, mx;
        uint32_t p;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p174_col[i][0] = 0.10f + 0.90f * r / mx;
        p174_col[i][1] = 0.10f + 0.90f * g / mx;
        p174_col[i][2] = 0.10f + 0.90f * b / mx;
    }
}

/* Clear a thin slab lying ACROSS the cord: the casing that makes an over-cord
   cut whatever passes beneath it. It is a slab and not a disc because a disc
   would also erase the same cord's own previous step. */
static void p174_cut(float cx, float cy, float ux, float uy,
                     float alen, float awid)
{
    float au = ux < 0.0f ? -ux : ux, av = uy < 0.0f ? -uy : uy;
    float ex = alen * au + awid * av, ey = alen * av + awid * au;
    int x0 = (int)(cx - ex), x1 = (int)(cx + ex) + 1;
    int y0 = (int)(cy - ey), y1 = (int)(cy + ey) + 1;
    int x, y;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > P174_W) x1 = P174_W; if (y1 > P174_H) y1 = P174_H;
    for (y = y0; y < y1; y++) {
        float dy = (float)y - cy;
        float *p = p174_acc + (y * P174_W + x0) * 3;
        for (x = x0; x < x1; x++, p += 3) {
            float dx = (float)x - cx;
            float a = dx * ux + dy * uy;
            float b = -dx * uy + dy * ux;
            if (a < 0.0f) a = -a;
            if (b < 0.0f) b = -b;
            if (a <= alen && b <= awid) { p[0] = 0.0f; p[1] = 0.0f; p[2] = 0.0f; }
        }
    }
}

/* soft round core with a shaded rim, so the cord reads as a tube */
static void p174_bead(float cx, float cy, float rad, const float *c, float w)
{
    int x0 = (int)(cx - rad), x1 = (int)(cx + rad) + 1;
    int y0 = (int)(cy - rad), y1 = (int)(cy + rad) + 1;
    int x, y;
    float ir2 = 1.0f / (rad * rad);
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > P174_W) x1 = P174_W; if (y1 > P174_H) y1 = P174_H;
    for (y = y0; y < y1; y++) {
        float dy = (float)y - cy;
        float *p = p174_acc + (y * P174_W + x0) * 3;
        for (x = x0; x < x1; x++, p += 3) {
            float dx = (float)x - cx;
            float q = 1.0f - (dx * dx + dy * dy) * ir2;
            float v;
            if (q <= 0.0f) continue;
            v = w * q * (0.45f + 0.55f * q);
            if (p[0] < c[0] * v) p[0] = c[0] * v;
            if (p[1] < c[1] * v) p[1] = c[1] * v;
            if (p[2] < c[2] * v) p[2] = c[2] * v;
        }
    }
}

static void p174_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P174_W * P174_H * 3; i++) {
        int ti = (int)(p174_acc[i] * 256.0f);
        p174_img[i] = p174_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p174_xmw != w) {
        free(p174_xm);
        p174_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p174_xm[x] = (int)(((long long)x * (P174_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p174_xmw = w;
    }
    jd_up_blit(&p174_up, fb, w, h, p174_img, P174_W, P174_H);
}

void pattern_174(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float cx = P174_W * 0.5f, cy = P174_H * 0.5f, S = (float)P174_H;
    float breathe;
    int k, i, j;
    (void)sl;
    if (!p174_ready || p174_seedc != seed) p174_setup(seed);
    p174_hues(pal);
    memset(p174_acc, 0, sizeof p174_acc);
    breathe = 1.0f + 0.045f * sinf(t * 0.00075f);

    for (k = P174_NR - 1; k >= 0; k--) {          /* outermost ring first    */
        int m = p174_m[k], ns;
        float R = p174_rad[k] * S * breathe;
        float A = p174_amp[k] * S;
        float f = (float)p174_p[k] + 1.0f / (float)m;
        float ph0 = p174_spd[k] * t;
        float wid = 2.1f + 0.55f * (float)k * 0.30f;
        float ord[P174_MAXM], rr[P174_MAXM], gx[P174_MAXM], gy[P174_MAXM];
        int idx[P174_MAXM];
        if (R > (float)P174_H * 0.50f) continue;
        ns = (int)(R * 4.4f) + 44;
        for (i = 0; i < ns; i++) {
            float th = P174_TAU * (float)i / (float)ns;
            float ct = cosf(th), st = sinf(th);
            for (j = 0; j < m; j++) {
                float a = f * th + P174_TAU * (float)j / (float)m + ph0;
                float rj = R + A * sinf(a);
                ord[j] = cosf(a);
                rr[j] = rj;
                gx[j] = cx + rj * ct;
                gy[j] = cy + rj * st;
                idx[j] = j;
            }
            for (j = 1; j < m; j++) {             /* insertion sort, back first */
                int q = idx[j], p2 = j - 1;
                while (p2 >= 0 && ord[idx[p2]] > ord[q]) { idx[p2 + 1] = idx[p2]; p2--; }
                idx[p2 + 1] = q;
            }
            for (j = 0; j < m; j++) {
                int s = idx[j];
                const float *c = p174_col[(int)((float)s / (float)m * 46.0f
                                                + (float)k * 9.0f) & 63];
                float depth = 0.62f + 0.38f * ord[s];
                float pulse = 0.72f + 0.28f * sinf(f * th * 2.0f - t * 0.010f
                                                   + (float)k * 1.3f);
                {   /* the casing must lie across the cord's TRUE direction of
                       travel, which is not the circle's tangent: r wobbles, so
                       dP/dtheta = (r' cos - r sin, r' sin + r cos) */
                    float rp = A * f * ord[s];
                    float ux = rp * ct - rr[s] * st;
                    float uy = rp * st + rr[s] * ct;
                    float il = 1.0f / sqrtf(ux * ux + uy * uy + 1e-6f);
                    p174_cut(gx[s], gy[s], ux * il, uy * il, 0.60f, wid + 1.35f);
                }
                p174_bead(gx[s], gy[s], wid, c, 1.45f * depth * pulse);
            }
        }
    }
    p174_blit(fb, w, h);
}
