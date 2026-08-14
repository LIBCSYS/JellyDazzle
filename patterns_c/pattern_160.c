/* 160 Chrome Relief — the DOS bump-map trick, done properly.
 * A height field is built from six travelling plane waves on three hexagonal
 * axes plus one radial ring wave, so the relief is a slowly kneading hexagonal
 * quilt. Its analytic gradient gives the surface normal N = norm(-dH/dx,
 * -dH/dy, 1) — no differencing, no noise — and the frame is then LIT: three
 * coloured directional lights taken from three stops of the palette, each with
 * Lambert diffuse and a Blinn specular raised to the 32nd power, plus a chrome
 * environment term that samples the palette by the normal's tilt. The lights
 * rotate on slow incommensurate orbits, so highlights crawl across the relief
 * like sun on hammered metal. Full-bleed, glossy and saturated: this one is a
 * GROUND layer — the thing the sparse wire routines sit on top of. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p160_up;

#define CW 440
#define CH 330
#define NWV 6

static unsigned char p160_img[CW * CH * 3];
static float p160_sin[4096];
static float p160_rr[CW * CH], p160_ix[CW * CH], p160_iy[CW * CH];
static int *p160_xm;
static int p160_xmw;
static float p160_dx[NWV], p160_dy[NWV], p160_sp[NWV], p160_am[NWV];
static float p160_lc[3][3], p160_env[64][3], p160_amb[3];
static float p160_hue0, p160_huew, p160_radf, p160_rads;
static uint32_t p160_seedc;
static int p160_ready, p160_tabs;

static uint32_t p160_rs;
static float p160_rf(void)
{
    p160_rs ^= p160_rs << 13; p160_rs ^= p160_rs >> 17; p160_rs ^= p160_rs << 5;
    return (float)(p160_rs >> 8) * (1.0f / 16777216.0f);
}

static void p160_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static void p160_setup(uint32_t seed)
{
    int i, x, y;
    float th0;
    p160_rs = seed ? seed ^ 0xC410E5u : 0xC410E5u;
    p160_rf(); p160_rf();
    th0 = p160_rf() * 6.2831853f;
    for (i = 0; i < NWV; i++) {
        float a = th0 + (float)(i % 3) * 1.0471976f;
        float lam = (i < 3 ? 62.0f + p160_rf() * 70.0f : 30.0f + p160_rf() * 32.0f);
        float k = 6.2831853f / lam;
        p160_dx[i] = cosf(a) * k; p160_dy[i] = sinf(a) * k;
        p160_sp[i] = (p160_rf() - 0.5f) * 0.030f;
        p160_am[i] = (i < 3 ? 0.95f : 0.26f) * (0.6f + 0.6f * p160_rf());
    }
    p160_radf = 6.2831853f / (44.0f + p160_rf() * 55.0f);
    p160_rads = (p160_rf() - 0.5f) * 0.022f;
    p160_hue0 = p160_rf();
    p160_huew = 0.06f + p160_rf() * 0.36f;
    if (!p160_tabs) {
        for (i = 0; i < 4096; i++)
            p160_sin[i] = sinf((float)i * (6.2831853f / 4096.0f));
        for (y = 0; y < CH; y++)
            for (x = 0; x < CW; x++) {
                float ddx = (float)x + 0.5f - CW * 0.5f;
                float ddy = (float)y + 0.5f - CH * 0.5f;
                float r = sqrtf(ddx * ddx + ddy * ddy);
                int o = y * CW + x;
                if (r < 0.7f) r = 0.7f;
                p160_rr[o] = r;
                p160_ix[o] = ddx / r; p160_iy[o] = ddy / r;
            }
        p160_tabs = 1;
    }
    p160_ready = 1;
    p160_seedc = seed;
}

static void p160_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p160_xmw != w) {
        free(p160_xm);
        p160_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p160_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p160_xmw = w;
    }
    jd_up_blit(&p160_up, fb, w, h, p160_img, CW, CH);
}

void pattern_160(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float ph0[NWV], gxk[NWV], gyk[NWV];
    float lx[3], ly[3], lz[3], hx[3], hy[3], hz[3];
    float radp, radg, bump;
    int x, y, i, c;
    (void)sl;
    if (!p160_ready || p160_seedc != seed) p160_setup(seed);

    for (i = 0; i < 3; i++)
        p160_pal3(pal, p160_hue0 + p160_huew * (float)i * 0.5f, 0.95f, p160_lc[i]);
    for (i = 0; i < 64; i++)
        p160_pal3(pal, p160_hue0 + p160_huew * ((float)i / 63.0f) * 1.4f - 0.15f,
                  0.80f, p160_env[i]);
    p160_pal3(pal, p160_hue0 + 0.5f, 0.75f, p160_amb);

    bump = 0.85f + 0.30f * sinf(t * 0.00061f);
    for (i = 0; i < NWV; i++) {
        ph0[i] = t * p160_sp[i];
        gxk[i] = p160_am[i] * p160_dx[i] * bump * 12.0f;
        gyk[i] = p160_am[i] * p160_dy[i] * bump * 12.0f;
    }
    radp = t * p160_rads;
    radg = 0.55f * p160_radf * bump * 12.0f;

    for (i = 0; i < 3; i++) {
        float a = t * (0.0013f + 0.00051f * (float)i) + (float)i * 2.0944f;
        float el = 0.58f + 0.22f * sinf(t * 0.00037f + (float)i);
        float cxy = sqrtf(1.0f - el * el);
        float n;
        lx[i] = cosf(a) * cxy; ly[i] = sinf(a) * cxy; lz[i] = el;
        hx[i] = lx[i]; hy[i] = ly[i]; hz[i] = lz[i] + 1.0f;
        n = 1.0f / sqrtf(hx[i] * hx[i] + hy[i] * hy[i] + hz[i] * hz[i]);
        hx[i] *= n; hy[i] *= n; hz[i] *= n;
    }

    for (y = 0; y < CH; y++) {
        float ph[NWV];
        unsigned char *op = p160_img + y * CW * 3;
        const float *rp = p160_rr + y * CW;
        const float *ixp = p160_ix + y * CW, *iyp = p160_iy + y * CW;
        for (i = 0; i < NWV; i++) ph[i] = ph0[i] + p160_dy[i] * (float)y + 200.0f;
        for (x = 0; x < CW; x++) {
            float gx = 0.0f, gy = 0.0f, s, nx, ny, nz, col[3];
            int o = x;
            for (i = 0; i < NWV; i++) {
                int idx = (int)((ph[i] + p160_dx[i] * (float)x)
                                * (4096.0f / 6.2831853f)) & 4095;
                float cq = p160_sin[(idx + 1024) & 4095];
                gx += gxk[i] * cq; gy += gyk[i] * cq;
            }
            {
                int idx = (int)((rp[o] * p160_radf + radp)
                                * (4096.0f / 6.2831853f)) & 4095;
                float cq = p160_sin[(idx + 1024) & 4095] * radg;
                gx += ixp[o] * cq; gy += iyp[o] * cq;
            }
            s = 1.0f / sqrtf(gx * gx + gy * gy + 1.0f);
            nx = -gx * s; ny = -gy * s; nz = s;
            {
                int ei = (int)((nx * 0.5f + 0.5f) * 63.0f);
                const float *ev = p160_env[ei < 0 ? 0 : ei > 63 ? 63 : ei];
                float ef = 0.30f * (0.20f + 0.80f * (1.0f - nz));
                for (c = 0; c < 3; c++)
                    col[c] = 0.030f * p160_amb[c] + ef * ev[c];
            }
            for (i = 0; i < 3; i++) {
                float d = nx * lx[i] + ny * ly[i] + nz * lz[i];
                float hh = nx * hx[i] + ny * hy[i] + nz * hz[i];
                float sp;
                if (d < 0.0f) d = 0.0f;
                if (hh < 0.0f) hh = 0.0f;
                sp = hh * hh; sp *= sp; sp *= sp; sp *= sp; sp *= sp;
                d = d * d * 0.34f + sp * 0.95f;
                for (c = 0; c < 3; c++) col[c] += d * p160_lc[i][c];
            }
            for (c = 0; c < 3; c++) {
                float v = col[c] * 255.0f + 0.5f;
                op[x * 3 + c] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
            }
        }
    }
    p160_blit(fb, w, h);
}
