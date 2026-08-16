/* 054 Starfield Warp — hue-ringed hyperspace drift with wandering vanishing point.
 * Port of lab/patterns/054_starfield_warp/proto.py.
 * Low-res float accumulator repainted every frame, bilinear upscale to fb. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
static jd_up p054_up;

#define P54_LW 320
#define P54_LH 240
#define P54_CX 160.0f
#define P54_CY 120.0f
#define P54_TAU 6.28318530717958647692f

static float p54_acc[P54_LW * P54_LH * 3];
static unsigned char p54_img[P54_LW * P54_LH * 3];
static int *p54_xmap;
static int p54_xmap_w;

static void p54_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P54_LW && (unsigned)yi < P54_LH) {
            float *p = p54_acc + (yi * P54_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

/* hue-family color: HSV chroma blended 35% with a saturated palette sample */
static void p54_color(const uint32_t *pal, float hue, float sat, float val,
                      float *r, float *g, float *b)
{
    float h6, f, hr, hg, hb, vr, vg, vb, mx;
    int i;
    uint32_t p;
    hue -= floorf(hue);
    h6 = hue * 6.0f; i = (int)h6; f = h6 - (float)i;
    switch (i % 6) {
    case 0:  hr = 1.0f; hg = f; hb = 0.0f; break;
    case 1:  hr = 1.0f - f; hg = 1.0f; hb = 0.0f; break;
    case 2:  hr = 0.0f; hg = 1.0f; hb = f; break;
    case 3:  hr = 0.0f; hg = 1.0f - f; hb = 1.0f; break;
    case 4:  hr = f; hg = 0.0f; hb = 1.0f; break;
    default: hr = 1.0f; hg = 0.0f; hb = 1.0f - f; break;
    }
    p = pal[(int)(hue * 0.55f * 32767.0f) & JD_PAL_MASK];
    vr = (float)((p >> 16) & 255); vg = (float)((p >> 8) & 255); vb = (float)(p & 255);
    mx = vr > vg ? vr : vg; if (vb > mx) mx = vb;
    if (mx < 1.0f) mx = 1.0f;
    vr /= mx; vg /= mx; vb /= mx;
    hr = 0.65f * hr + 0.35f * vr;
    hg = 0.65f * hg + 0.35f * vg;
    hb = 0.65f * hb + 0.35f * vb;
    *r = val * ((1.0f - sat) + sat * hr);
    *g = val * ((1.0f - sat) + sat * hg);
    *b = val * ((1.0f - sat) + sat * hb);
}

static void p54_blit(uint32_t *fb, int w, int h)
{
    int i, x;
    int n = P54_LW * P54_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p54_acc[i] * 255.0f;
        p54_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p54_xmap_w != w) {
        free(p54_xmap);
        p54_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p54_xmap[x] = (int)(((long long)x * (P54_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p54_xmap_w = w;
    }
    jd_up_blit(&p054_up, fb, w, h, p54_img, P54_LW, P54_LH);
}

/* ------------- pattern state ------------- */
#define P54_N 750
#define P54_NS 10
#define P54_ZMAX 1.1f
#define P54_V 0.0022f
#define P54_F 170.0f

static int p54_init_done;
static float p54_ux[P54_N], p54_uy[P54_N], p54_z0[P54_N];
static float p54_tw[P54_N], p54_hue[P54_N];
/* nebula ground: two gaussian lobes, precomputed unit fields */
static float p54_n1[P54_LW * P54_LH], p54_n2[P54_LW * P54_LH];
static float p54_neb_a = -1e9f, p54_neb_b = -1e9f;

static uint32_t p54_rs;
static float p54_rf(void)
{
    p54_rs ^= p54_rs << 13; p54_rs ^= p54_rs >> 17; p54_rs ^= p54_rs << 5;
    return (float)(p54_rs >> 8) * (1.0f / 16777216.0f);
}
/* box-muller normal */
static float p54_rn(float sd)
{
    float u1 = p54_rf(), u2 = p54_rf();
    if (u1 < 1e-6f) u1 = 1e-6f;
    return sd * sqrtf(-2.0f * logf(u1)) * cosf(P54_TAU * u2);
}

static void p54_init(void)
{
    int i;
    p54_rs = 0x54D1E205u;
    for (i = 0; i < P54_N; i++) {
        p54_ux[i] = p54_rn(0.50f);
        p54_uy[i] = p54_rn(0.38f);
        p54_z0[i] = p54_rf() * 1.1f;
        p54_tw[i] = p54_rf() * P54_TAU;
        p54_hue[i] = atan2f(p54_uy[i], p54_ux[i]) * (1.0f / P54_TAU)
                     + p54_z0[i] * 0.12f;
    }
    p54_init_done = 1;
}

/* rebuild the nebula lobes only when their drift has moved appreciably */
static void p54_nebula(float a, float b)
{
    int x, y;
    if (fabsf(a - p54_neb_a) < 0.35f && fabsf(b - p54_neb_b) < 0.35f) return;
    p54_neb_a = a; p54_neb_b = b;
    for (y = 0; y < P54_LH; y++) {
        float fy = (float)y;
        float ay = (fy - 70.0f) * (1.0f / 95.0f);
        float by = (fy - 180.0f) * (1.0f / 100.0f);
        float ay2 = ay * ay, by2 = by * by;
        for (x = 0; x < P54_LW; x++) {
            float ax = ((float)x - a) * (1.0f / 130.0f);
            float bx = ((float)x - b) * (1.0f / 120.0f);
            p54_n1[y * P54_LW + x] = expf(-(ax * ax + ay2));
            p54_n2[y * P54_LW + x] = expf(-(bx * bx + by2));
        }
    }
}

void pattern_054(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float tt = (float)(frame % 1048576) + 500.0f;
    float cx, cy, hshift;
    int i, si, n;
    (void)sl; (void)seed;
    if (!p54_init_done) p54_init();

    p54_nebula(90.0f + 30.0f * sinf(tt * 0.002f),
               240.0f - 25.0f * sinf(tt * 0.0016f + 2.0f));

    n = P54_LW * P54_LH;
    for (i = 0; i < n; i++) {
        float a = p54_n1[i], b = p54_n2[i];
        p54_acc[i * 3 + 0] = 0.10f * a + 0.02f * b;
        p54_acc[i * 3 + 1] = 0.02f * a + 0.06f * b;
        p54_acc[i * 3 + 2] = 0.16f * a + 0.13f * b + 0.03f;
    }

    cx = P54_CX + 16.0f * sinf(tt * 0.0045f);
    cy = P54_CY + 11.0f * sinf(tt * 0.0031f + 1.2f);
    hshift = tt * 0.0005f;

    for (i = 0; i < P54_N; i++) {
        float z = p54_z0[i] - P54_V * tt;
        float zp, cr, cg, cb, tk;
        z = fmodf(z, P54_ZMAX);
        if (z < 0.0f) z += P54_ZMAX;
        z += 0.07f;
        zp = z + P54_V * 9.0f;
        tk = 0.8f + 0.2f * sinf(tt * 0.05f + p54_tw[i]);
        for (si = 0; si < P54_NS; si++) {
            float f = (float)si * (1.0f / (float)(P54_NS - 1));
            float zi = z * (1.0f - f) + zp * f;
            float inv = 1.0f / zi;
            float px = cx + p54_ux[i] * P54_F * inv;
            float py = cy + p54_uy[i] * P54_F * inv;
            float bright, sat;
            if (px < -2.0f || px > P54_LW + 2.0f ||
                py < -2.0f || py > P54_LH + 2.0f) continue;
            bright = 0.34f * inv;
            if (bright < 0.05f) bright = 0.05f;
            else if (bright > 1.3f) bright = 1.3f;
            bright *= (1.0f - 0.70f * f) * tk;
            sat = zi * 1.15f;
            if (sat < 0.15f) sat = 0.15f; else if (sat > 0.8f) sat = 0.8f;
            p54_color(pal, p54_hue[i] + hshift, sat, 1.0f, &cr, &cg, &cb);
            p54_splat(px, py, cr, cg, cb, bright);
        }
    }
    p54_blit(fb, w, h);
}
