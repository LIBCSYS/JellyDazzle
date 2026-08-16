/* 153 Loxodrome Drift — the streamlines of a Moebius flow between two poles.
 * A loxodromic Moebius map has two fixed points p,q; in the log coordinate
 * w = log((z-p)/(z-q)) it is a pure translation by log(lambda) = a + i*theta,
 * so its orbits are straight lines in a cylinder and double spirals on screen —
 * every filament leaves one pole and winds into the other. The frame draws a
 * family of those spirals directly (z = (p - q e^w)/(1 - e^w)), with light
 * pulses running along each filament toward the sink, the two poles slowly
 * drifting, and the whole family precessing. Black between the filaments:
 * an overlay layer that reads as luminous hair combed around two singularities. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p153_up;

#define CW 480
#define CH 360
#define NLINE 26
#define NS 520

static float p153_acc[CW * CH * 3];
static float p153_tmp[CW * CH * 3];
static unsigned char p153_img[CW * CH * 3];
static unsigned char p153_tone[1024];
static int *p153_xm;
static int p153_xmw;
static float p153_hue0, p153_huew, p153_aa, p153_th, p153_sep;
static uint32_t p153_seedc;
static int p153_ready;
static float p153_hue[64][3];

static uint32_t p153_rs;
static float p153_rf(void)
{
    p153_rs ^= p153_rs << 13; p153_rs ^= p153_rs >> 17; p153_rs ^= p153_rs << 5;
    return (float)(p153_rs >> 8) * (1.0f / 16777216.0f);
}

static void p153_setup(uint32_t seed)
{
    int i;
    p153_rs = seed ? seed ^ 0x10C0D3u : 0x10C0D3u;
    p153_rf(); p153_rf();
    p153_hue0 = p153_rf();
    p153_huew = 0.06f + p153_rf() * 0.40f;
    p153_aa = 0.22f + p153_rf() * 0.24f;              /* radial gain        */
    p153_th = (p153_rf() < 0.5f ? -1.0f : 1.0f) * (0.55f + p153_rf() * 0.75f);
    p153_sep = 0.42f + p153_rf() * 0.30f;
    if (!p153_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (4.4f / 1024.0f)));
            p153_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p153_ready = 1;
    }
    p153_seedc = seed;
}

static void p153_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 64; i++) {
        float hue = p153_hue0 + p153_huew * ((float)i / 63.0f);
        uint32_t p;
        float r, g, b, mx;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p153_hue[i][0] = 0.10f + 0.90f * r / mx;
        p153_hue[i][1] = 0.10f + 0.90f * g / mx;
        p153_hue[i][2] = 0.10f + 0.90f * b / mx;
    }
}

static void p153_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= CW - 1 || yi >= CH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p153_acc + (yi * CW + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += CW * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p153_seg(float x0, float y0, float x1, float y1, const float *c, float w)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n, i;
    float sx, sy, ww;
    if (len > 160.0f) return;
    n = (int)(len * 1.25f) + 1;
    sx = dx / (float)n; sy = dy / (float)n;
    ww = w * (len / (float)n + 0.35f);
    for (i = 0; i < n; i++)
        p153_splat(x0 + sx * (float)i, y0 + sy * (float)i, c, ww);
}

static void p153_blur(void)
{
    int y, x, c;
    for (y = 1; y < CH - 1; y++)
        for (x = 1; x < CW - 1; x++) {
            int o = (y * CW + x) * 3;
            for (c = 0; c < 3; c++)
                p153_tmp[o + c] = p153_acc[o + c] * 0.48f
                    + 0.13f * (p153_acc[o + c - 3] + p153_acc[o + c + 3]
                             + p153_acc[o + c - CW * 3] + p153_acc[o + c + CW * 3]);
        }
    for (y = 1; y < CH - 1; y++)
        memcpy(p153_acc + (y * CW + 1) * 3, p153_tmp + (y * CW + 1) * 3,
               sizeof(float) * 3 * (CW - 2));
}

static void p153_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < CW * CH * 3; i++) {
        int ti = (int)(p153_acc[i] * 256.0f);
        p153_img[i] = p153_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p153_xmw != w) {
        free(p153_xm);
        p153_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p153_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p153_xmw = w;
    }
    jd_up_blit(&p153_up, fb, w, h, p153_img, CW, CH);
}

void pattern_153(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float L, dux, duy, nux, nuy, period, sc, ca, sa;
    float px, py, qx, qy, spin;
    int m, i;
    (void)sl;
    if (!p153_ready || p153_seedc != seed) p153_setup(seed);
    p153_hues(pal);
    memset(p153_acc, 0, sizeof p153_acc);

    L = sqrtf(p153_aa * p153_aa + p153_th * p153_th);
    dux = p153_aa / L; duy = p153_th / L;          /* flow direction in w    */
    nux = -duy; nuy = dux;                         /* across the flow        */
    period = 6.2831853f * p153_aa / L;             /* distinct streamlines   */

    spin = t * 0.0021f;
    ca = cosf(spin); sa = sinf(spin);
    sc = CH * (0.40f + 0.030f * sinf(t * 0.00097f));
    {   /* the two poles, drifting slowly around the centre */
        float d = p153_sep * (1.0f + 0.14f * sinf(t * 0.00061f));
        px = d * ca; py = d * sa;
        qx = -px; qy = -py;
    }

    for (m = 0; m < NLINE; m++) {
        float c0 = period * (((float)m + 0.5f) / NLINE) + t * 0.00042f;
        float px0 = -1e9f, py0 = -1e9f;
        int have = 0;
        for (i = 0; i <= NS; i++) {
            /* walk the log coordinate directly so every filament spans the
               same pole-to-pole range whatever the pitch of the spiral */
            float u = -3.7f + (7.4f / NS) * (float)i;
            float s = (u - c0 * nux) / dux;
            float v = c0 * nuy + s * duy;
            float eu, er, ei, dr, di, dd, zx, zy, X, Y;
            float env, pulse, wgt;
            const float *col;
            eu = expf(u);
            er = eu * cosf(v); ei = eu * sinf(v);
            dr = 1.0f - er; di = -ei;
            dd = dr * dr + di * di;
            if (dd < 1e-6f) { have = 0; continue; }
            /* z = (p - q*E) / (1 - E) */
            {
                float nr = px - (qx * er - qy * ei);
                float ni = py - (qx * ei + qy * er);
                zx = (nr * dr + ni * di) / dd;
                zy = (ni * dr - nr * di) / dd;
            }
            X = CW * 0.5f + zx * sc;
            Y = CH * 0.5f + zy * sc;
            env = expf(-(u * u) * 0.135f);
            pulse = 0.34f + 0.66f * (0.5f + 0.5f * sinf(u * 2.3f - t * 0.021f
                                                        + (float)m * 0.55f));
            wgt = 0.95f * env * pulse * pulse;
            col = p153_hue[(int)((u + 3.7f) * (63.0f / 7.4f)) & 63];
            if (have && wgt > 0.01f) p153_seg(px0, py0, X, Y, col, wgt);
            px0 = X; py0 = Y; have = 1;
        }
    }
    p153_blur();
    p153_blit(fb, w, h);
}
