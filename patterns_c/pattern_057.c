/* 057 Galaxy Pinwheel — two-arm logarithmic spiral of 3000 stars, tilted and turning.
 * Port of lab/patterns/057_galaxy_pinwheel/proto.py.
 * Low-res float accumulator repainted every frame, bilinear upscale to fb. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P57_LW 320
#define P57_LH 240
#define P57_CX 160.0f
#define P57_CY 120.0f
#define P57_TAU 6.28318530717958647692f

static float p57_acc[P57_LW * P57_LH * 3];
static unsigned char p57_img[P57_LW * P57_LH * 3];
static int *p57_xmap;
static int p57_xmap_w;

static void p57_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P57_LW && (unsigned)yi < P57_LH) {
            float *p = p57_acc + (yi * P57_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

static void p57_color(const uint32_t *pal, float hue, float sat, float val,
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

static void p57_blit(uint32_t *fb, int w, int h)
{
    int i, x, y;
    int n = P57_LW * P57_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p57_acc[i] * 255.0f;
        p57_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p57_xmap_w != w) {
        free(p57_xmap);
        p57_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p57_xmap[x] = (int)(((long long)x * (P57_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p57_xmap_w = w;
    }
    for (y = 0; y < h; y++) {
        int sy = (int)(((long long)y * (P57_LH - 1) << 8) / (h > 1 ? h - 1 : 1));
        int y0 = sy >> 8, fy = sy & 255;
        int y1 = y0 + 1 < P57_LH ? y0 + 1 : P57_LH - 1;
        const unsigned char *r0 = p57_img + y0 * P57_LW * 3;
        const unsigned char *r1 = p57_img + y1 * P57_LW * 3;
        uint32_t *dst = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int sx = p57_xmap[x];
            int x0 = sx >> 8, fx = sx & 255;
            int x1 = x0 + 1 < P57_LW ? x0 + 1 : P57_LW - 1;
            int o0 = x0 * 3, o1 = x1 * 3, c, out[3];
            for (c = 0; c < 3; c++) {
                int top = r0[o0 + c] + (((r0[o1 + c] - r0[o0 + c]) * fx) >> 8);
                int bot = r1[o0 + c] + (((r1[o1 + c] - r1[o0 + c]) * fx) >> 8);
                out[c] = top + (((bot - top) * fy) >> 8);
            }
            dst[x] = 0xFF000000u | ((uint32_t)out[0] << 16) |
                     ((uint32_t)out[1] << 8) | (uint32_t)out[2];
        }
    }
}

/* ------------- pattern state ------------- */
#define P57_N 3000
#define P57_NF 9

static int p57_init_done;
static float p57_r[P57_N], p57_th0[P57_N], p57_hj[P57_N], p57_tw[P57_N];
static float p57_spir[P57_N], p57_om[P57_N];
static float p57_hue[P57_N], p57_sat[P57_N], p57_v0[P57_N];
static float p57_fsx[P57_NF], p57_fsy[P57_NF], p57_fsp[P57_NF];
static float p57_bg[P57_LW * P57_LH * 3];

static uint32_t p57_rs;
static float p57_rf(void)
{
    p57_rs ^= p57_rs << 13; p57_rs ^= p57_rs >> 17; p57_rs ^= p57_rs << 5;
    return (float)(p57_rs >> 8) * (1.0f / 16777216.0f);
}
static float p57_rn(float sd)
{
    float u1 = p57_rf(), u2 = p57_rf();
    if (u1 < 1e-6f) u1 = 1e-6f;
    return sd * sqrtf(-2.0f * logf(u1)) * cosf(P57_TAU * u2);
}

static void p57_init(void)
{
    int i, x, y;
    p57_rs = 0x57A1E00Du;
    for (i = 0; i < P57_N; i++) {
        float rr = 132.0f * powf(p57_rf(), 0.72f);
        float arm = (p57_rf() < 0.5f) ? 0.0f : 3.14159265358979f;
        p57_r[i] = rr;
        p57_th0[i] = arm + p57_rn(0.30f) * (0.35f + rr / 90.0f);
        p57_hj[i] = p57_rn(0.025f);
        p57_tw[i] = p57_rf() * P57_TAU;
        p57_spir[i] = 2.5f * logf(1.0f + rr / 13.0f);
        p57_om[i] = 0.0042f / (0.45f + rr / 60.0f);
        p57_hue[i] = 0.64f - 0.56f * expf(-rr / 55.0f) + p57_hj[i];
        p57_sat[i] = 0.80f - 0.45f * expf(-rr / 22.0f);
        p57_v0[i] = 0.38f + 0.62f * expf(-rr / 80.0f);
    }
    for (i = 0; i < P57_NF; i++) {
        p57_fsx[i] = 15.0f + p57_rf() * (P57_LW - 30.0f);
        p57_fsy[i] = 12.0f + p57_rf() * (P57_LH - 24.0f);
        p57_fsp[i] = p57_rf() * P57_TAU;
    }
    /* static deep-space ground + core halo */
    for (y = 0; y < P57_LH; y++) {
        float dy = ((float)y - P57_CY) / 0.62f;
        for (x = 0; x < P57_LW; x++) {
            float dx = (float)x - P57_CX;
            float d = sqrtf(dx * dx + dy * dy);
            float e26 = d / 26.0f, e22 = d / 22.0f;
            float *p = p57_bg + (y * P57_LW + x) * 3;
            p[0] = 0.015f + 0.16f * expf(-e26 * e26);
            p[1] = 0.10f * expf(-e22 * e22);
            p[2] = 0.05f + 0.10f * expf(-d / 60.0f);
        }
    }
    p57_init_done = 1;
}

void pattern_057(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float tt = (float)(frame % 1048576) + 400.0f;
    float axis, ca, sa, hshift;
    int i, k, n;
    (void)sl; (void)seed;
    if (!p57_init_done) p57_init();

    n = P57_LW * P57_LH * 3;
    for (i = 0; i < n; i++) p57_acc[i] = p57_bg[i];

    axis = tt * 0.00055f;
    ca = cosf(axis); sa = sinf(axis);
    hshift = tt * 0.00025f;

    for (i = 0; i < P57_N; i++) {
        float th = p57_th0[i] + p57_spir[i] + tt * p57_om[i];
        float rr = p57_r[i];
        float xr = rr * cosf(th);
        float yr = rr * sinf(th) * 0.62f;
        float x = P57_CX + xr * ca - yr * sa;
        float y = P57_CY + (xr * sa + yr * ca) * 0.92f;
        float val, cr, cg, cb;
        if (x < -1.0f || x > P57_LW + 1.0f || y < -1.0f || y > P57_LH + 1.0f) continue;
        val = p57_v0[i] * (0.78f + 0.22f * sinf(tt * 0.05f + p57_tw[i]));
        p57_color(pal, p57_hue[i] + hshift, p57_sat[i], 1.0f, &cr, &cg, &cb);
        p57_splat(x, y, cr, cg, cb, val * 1.15f);
    }

    /* foreground cross-sparkle stars */
    {
        static const int ddx[5] = {0, 2, -2, 0, 0};
        static const int ddy[5] = {0, 0, 0, 2, -2};
        static const float kk[5] = {1.0f, 0.4f, 0.4f, 0.4f, 0.4f};
        for (i = 0; i < P57_NF; i++) {
            float fv = 0.35f + 0.30f * sinf(tt * 0.02f + p57_fsp[i]);
            for (k = 0; k < 5; k++)
                p57_splat(p57_fsx[i] + (float)ddx[k], p57_fsy[i] + (float)ddy[k],
                          fv, fv, fv * 1.15f, kk[k]);
        }
    }
    p57_blit(fb, w, h);
}
