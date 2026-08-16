/* 155 Knot Loom — a 3-D torus knot drawn as glowing wire, morphing between
 * two knot types. Each strand is the classic parametrisation
 *   x = (R + r cos q phi) cos p phi,  y = (R + r cos q phi) sin p phi,
 *   z = r sin q phi
 * evaluated for a (p1,q1) knot and a (p2,q2) knot on the same phi grid and
 * LERPed by a slow cosine, so one closed curve melts continuously into the
 * other — no cut is ever visible because both endpoints of the blend are
 * closed curves. Three strands at staggered tube radii and phases are rotated
 * on two axes, perspective-projected, and additively splatted with depth
 * shading: near wire is bright and warm, far wire dim and cool, so the braid
 * reads as solid 3-D. Line art on black — the sparsest layer in the set. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p155_up;

#define CW 480
#define CH 360
#define NSAMP 900
#define NSTR 3

static float p155_acc[CW * CH * 3];
static float p155_tmp[CW * CH * 3];
static unsigned char p155_img[CW * CH * 3];
static unsigned char p155_tone[1024];
static int *p155_xm;
static int p155_xmw;
static float p155_hue[48][3];
static float p155_hue0, p155_huew;
static int p155_p1, p155_q1, p155_p2, p155_q2;
static uint32_t p155_seedc;
static int p155_ready;

static uint32_t p155_rs;
static float p155_rf(void)
{
    p155_rs ^= p155_rs << 13; p155_rs ^= p155_rs >> 17; p155_rs ^= p155_rs << 5;
    return (float)(p155_rs >> 8) * (1.0f / 16777216.0f);
}

static void p155_setup(uint32_t seed)
{
    static const int kp[8] = {2, 3, 3, 2, 5, 3, 4, 2};
    static const int kq[8] = {3, 2, 4, 5, 2, 5, 3, 7};
    int a, b, i;
    p155_rs = seed ? seed ^ 0x4B0A07u : 0x4B0A07u;
    p155_rf(); p155_rf();
    a = (int)(p155_rf() * 8.0f); if (a > 7) a = 7;
    b = (int)(p155_rf() * 8.0f); if (b > 7) b = 7;
    if (b == a) b = (a + 3) & 7;
    p155_p1 = kp[a]; p155_q1 = kq[a];
    p155_p2 = kp[b]; p155_q2 = kq[b];
    p155_hue0 = p155_rf();
    p155_huew = 0.05f + p155_rf() * 0.34f;
    if (!p155_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (8.5f / 1024.0f)));
            p155_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p155_ready = 1;
    }
    p155_seedc = seed;
}

static void p155_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 48; i++) {
        float hue = p155_hue0 + p155_huew * ((float)i / 47.0f);
        uint32_t p; float r, g, b, mx;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p155_hue[i][0] = 0.12f + 0.88f * r / mx;
        p155_hue[i][1] = 0.12f + 0.88f * g / mx;
        p155_hue[i][2] = 0.12f + 0.88f * b / mx;
    }
}

static void p155_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= CW - 1 || yi >= CH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p155_acc + (yi * CW + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += CW * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p155_seg(float x0, float y0, float x1, float y1, const float *c, float w)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n, i;
    float sx, sy, ww;
    if (len > 300.0f) return;
    n = (int)(len * 1.2f) + 1;
    sx = dx / (float)n; sy = dy / (float)n;
    ww = w * (len / (float)n + 0.30f);
    for (i = 0; i < n; i++)
        p155_splat(x0 + sx * (float)i, y0 + sy * (float)i, c, ww);
}

static void p155_blur(void)
{
    int y, x, c;
    for (y = 1; y < CH - 1; y++)
        for (x = 1; x < CW - 1; x++) {
            int o = (y * CW + x) * 3;
            for (c = 0; c < 3; c++)
                p155_tmp[o + c] = p155_acc[o + c] * 0.46f
                    + 0.135f * (p155_acc[o + c - 3] + p155_acc[o + c + 3]
                              + p155_acc[o + c - CW * 3] + p155_acc[o + c + CW * 3]);
        }
    for (y = 1; y < CH - 1; y++)
        memcpy(p155_acc + (y * CW + 1) * 3, p155_tmp + (y * CW + 1) * 3,
               sizeof(float) * 3 * (CW - 2));
}

static void p155_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < CW * CH * 3; i++) {
        int ti = (int)(p155_acc[i] * 256.0f);
        p155_img[i] = p155_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p155_xmw != w) {
        free(p155_xm);
        p155_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p155_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p155_xmw = w;
    }
    jd_up_blit(&p155_up, fb, w, h, p155_img, CW, CH);
}

void pattern_155(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, al, ca, sa, cb, sb, sc, dist;
    int s, i;
    (void)sl;
    if (!p155_ready || p155_seedc != seed) p155_setup(seed);
    p155_hues(pal);
    memset(p155_acc, 0, sizeof p155_acc);

    al = 0.5f - 0.5f * cosf(t * 0.00085f);
    al = al * al * (3.0f - 2.0f * al);
    al = al * al * (3.0f - 2.0f * al);
    ca = cosf(t * 0.0031f); sa = sinf(t * 0.0031f);
    cb = cosf(t * 0.0019f + 0.7f); sb = sinf(t * 0.0019f + 0.7f);
    sc = (float)CH * 0.290f * (1.0f + 0.05f * sinf(t * 0.0011f));
    dist = 6.4f;

    for (s = 0; s < NSTR; s++) {
        float tr = 0.34f + 0.13f * (float)s;
        float phs = (float)s * 0.42f;
        float px0 = 0.0f, py0 = 0.0f;
        int have = 0;
        for (i = 0; i <= NSAMP; i++) {
            float ph = (float)i * (6.2831853f / NSAMP) + phs;
            float ax, ay, az, bx, by, bz, X, Y, Z, xr, yr, zr, pz, sx, sy;
            float wgt, dep;
            const float *col;
            {
                float u = ph * (float)p155_p1, v = ph * (float)p155_q1;
                float rr = 1.0f + tr * cosf(v);
                ax = rr * cosf(u); ay = rr * sinf(u); az = tr * sinf(v);
            }
            {
                float u = ph * (float)p155_p2, v = ph * (float)p155_q2;
                float rr = 1.0f + tr * cosf(v);
                bx = rr * cosf(u); by = rr * sinf(u); bz = tr * sinf(v);
            }
            X = ax + (bx - ax) * al;
            Y = ay + (by - ay) * al;
            Z = az + (bz - az) * al;
            /* rotate about X then Y */
            yr = Y * ca - Z * sa; zr = Y * sa + Z * ca;
            xr = X * cb + zr * sb; zr = -X * sb + zr * cb;
            pz = dist - zr;
            if (pz < 1.2f) { have = 0; continue; }
            {
                float k = dist / pz;
                sx = CW * 0.5f + xr * sc * k * 1.30f;
                sy = CH * 0.5f + yr * sc * k * 1.30f;
                dep = (zr + 1.6f) * 0.31f;          /* 0 far .. 1 near      */
                if (dep < 0.0f) dep = 0.0f; if (dep > 1.0f) dep = 1.0f;
                wgt = (0.13f + 1.30f * dep * dep * dep) * k * 1.35f;
            }
            col = p155_hue[(int)(dep * 47.0f) & 47];
            if (have) p155_seg(px0, py0, sx, sy, col, wgt);
            px0 = sx; py0 = sy; have = 1;
        }
    }
    p155_blur();
    p155_blit(fb, w, h);
}
