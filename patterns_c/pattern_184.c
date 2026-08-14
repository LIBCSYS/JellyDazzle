/* 184 Caustic Billiard — a long exposure of balls bouncing forever inside one
 * table. The wall is the implicit superellipse  (|x|/a)^p + (|y|/b)^p = 1;
 * each frame every ball is ray-marched to the wall, the chord is drawn, and the
 * velocity is reflected in the true gradient normal, d' = d - 2(d.n)n. Nothing
 * is faked: at p = 2 the table is an ellipse, the billiard is integrable, and
 * every orbit is tangent to one confocal caustic — so the exposure grows a
 * bright rim around an untouched elliptical hole. Push p away from 2 and the
 * integrability breaks; the caustics blur into whorls and the tangle slowly
 * fills. Chords are stamped at low weight into a persistent canvas that decays
 * 0.3% per frame, so structure builds where orbits crowd, exactly as light
 * does. The drawing frame precesses about half a degree per second, so fresh
 * strokes lay down rotated against the fading ones and the whole exposure
 * turns into a rosette. Four-fold mirrored (the table's own symmetry group).
 * Accumulator: clears at sl == 0 and builds for the whole segment. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p184_up;

#define CW 480
#define CH 360
#define NBALL 3

static float p184_acc[CW * CH * 3];
static unsigned char p184_img[CW * CH * 3];
static unsigned char p184_tone[2048];
static int *p184_xm;
static int p184_xmw;
static float p184_bx[NBALL], p184_by[NBALL], p184_vx[NBALL], p184_vy[NBALL];
static float p184_ta, p184_tb, p184_tp, p184_hue0, p184_huew, p184_prec, p184_hspd;
static uint32_t p184_seedc;
static int p184_ready, p184_tabs, p184_last;

static uint32_t p184_rs;
static float p184_rf(void)
{
    p184_rs ^= p184_rs << 13; p184_rs ^= p184_rs >> 17; p184_rs ^= p184_rs << 5;
    return (float)(p184_rs >> 8) * (1.0f / 16777216.0f);
}

static void p184_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static float p184_F(float x, float y)
{
    float u = fabsf(x) / p184_ta, v = fabsf(y) / p184_tb;
    return powf(u, p184_tp) + powf(v, p184_tp) - 1.0f;
}

static void p184_grad(float x, float y, float *gx, float *gy)
{
    float e = 0.0025f;
    *gx = p184_F(x + e, y) - p184_F(x - e, y);
    *gy = p184_F(x, y + e) - p184_F(x, y - e);
}

static void p184_reset(uint32_t seed)
{
    int i;
    p184_rs = seed ? seed ^ 0xB11A4Du : 0xB11A4Du;
    p184_rf(); p184_rf();
    p184_ta = 1.20f + p184_rf() * 0.28f;
    p184_tb = 0.86f + p184_rf() * 0.14f;
    p184_tp = (p184_rf() < 0.45f) ? 2.0f : (1.72f + p184_rf() * 1.35f);
    p184_hue0 = p184_rf();
    p184_huew = 0.10f + p184_rf() * 0.46f;
    p184_prec = (p184_rf() < 0.5f ? -1.0f : 1.0f) * (0.00055f + p184_rf() * 0.00075f);
    p184_hspd = 0.00042f + p184_rf() * 0.00060f;
    for (i = 0; i < NBALL; i++) {
        float a = p184_rf() * 6.2831853f;
        float d = 0.14f + 0.62f * ((float)i / NBALL) + p184_rf() * 0.13f;
        float b = p184_rf() * 6.2831853f;
        p184_bx[i] = cosf(a) * d * p184_ta * 0.7f;
        p184_by[i] = sinf(a) * d * p184_tb * 0.7f;
        p184_vx[i] = cosf(b); p184_vy[i] = sinf(b);
    }
    memset(p184_acc, 0, sizeof p184_acc);
    if (!p184_tabs) {
        for (i = 0; i < 2048; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (5.0f / 2048.0f)));
            p184_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p184_tabs = 1;
    }
    p184_ready = 1;
    p184_seedc = seed;
}

static void p184_splat(float x, float y, const float *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 1.0f || y < 1.0f || xi >= CW - 2 || yi >= CH - 2) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p184_acc + (yi * CW + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * wgt; w1 = fx * (1.0f - fy) * wgt;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += CW * 3;
    w0 = (1.0f - fx) * fy * wgt; w1 = fx * fy * wgt;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p184_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p184_xmw != w) {
        free(p184_xm);
        p184_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p184_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p184_xmw = w;
    }
    jd_up_blit(&p184_up, fb, w, h, p184_img, CW, CH);
}

void pattern_184(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, cs, sn, cx, cy, sc, col[3];
    int i, j, o;
    if (!p184_ready || p184_seedc != seed || sl == 0 || sl < p184_last)
        p184_reset(seed);
    p184_last = sl;

    cs = cosf(t * p184_prec); sn = sinf(t * p184_prec);
    cx = CW * 0.5f; cy = CH * 0.5f;
    sc = 150.0f + 5.0f * sinf(t * 0.00047f);

    for (o = 0; o < CW * CH * 3; o++) p184_acc[o] *= 0.9948f;

    for (i = 0; i < NBALL; i++) {
        float x = p184_bx[i], y = p184_by[i], vx = p184_vx[i], vy = p184_vy[i];
        float lo = 0.004f, hi = 0.004f, len, gx, gy, gl, dn;
        int ns;
        p184_pal3(pal, p184_hue0 + p184_huew * (0.5f + 0.5f *
                  sinf(t * p184_hspd + (float)i * 2.1f)), 0.90f, col);
        /* march to the wall, then bisect */
        while (hi < 4.0f && p184_F(x + vx * hi, y + vy * hi) < 0.0f) {
            lo = hi; hi += 0.020f;
        }
        for (j = 0; j < 24; j++) {
            float mid = 0.5f * (lo + hi);
            if (p184_F(x + vx * mid, y + vy * mid) < 0.0f) lo = mid; else hi = mid;
        }
        len = lo;
        ns = (int)(len * sc * 1.35f);
        if (ns < 4) ns = 4; else if (ns > 900) ns = 900;
        for (j = 0; j <= ns; j++) {
            float s = len * (float)j / (float)ns;
            float px = x + vx * s, py = y + vy * s;
            float rx = px * cs - py * sn, ry = px * sn + py * cs;
            float wq = 0.0125f;
            p184_splat(cx + rx * sc, cy + ry * sc, col, wq);
            p184_splat(cx - rx * sc, cy - ry * sc, col, wq);
            p184_splat(cx + rx * sc, cy - ry * sc, col, wq);
            p184_splat(cx - rx * sc, cy + ry * sc, col, wq);
        }
        x += vx * len; y += vy * len;
        p184_grad(x, y, &gx, &gy);
        gl = gx * gx + gy * gy;
        if (gl > 1e-12f) {
            gl = 1.0f / sqrtf(gl); gx *= gl; gy *= gl;
            dn = 2.0f * (vx * gx + vy * gy);
            vx -= dn * gx; vy -= dn * gy;
            gl = 1.0f / sqrtf(vx * vx + vy * vy); vx *= gl; vy *= gl;
        } else { vx = -vx; vy = -vy; }
        p184_bx[i] = x + vx * 0.0035f;
        p184_by[i] = y + vy * 0.0035f;
        p184_vx[i] = vx; p184_vy[i] = vy;
    }

    for (o = 0; o < CW * CH * 3; o++) {
        int v = (int)(p184_acc[o] * 1000.0f);
        p184_img[o] = p184_tone[v < 0 ? 0 : v > 2047 ? 2047 : v];
    }
    p184_blit(fb, w, h);
}
