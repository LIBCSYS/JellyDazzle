/* 060 Meteor Veil — mirrored bolide shower over a twinkling twilight sky.
 * Port of lab/patterns/060_meteor_veil/proto.py.
 * Low-res float accumulator repainted every frame, bilinear upscale to fb. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
static jd_up p060_up;

#define P60_LW 320
#define P60_LH 240
#define P60_CX 160.0f
#define P60_CY 120.0f
#define P60_TAU 6.28318530717958647692f

static float p60_acc[P60_LW * P60_LH * 3];
static unsigned char p60_img[P60_LW * P60_LH * 3];
static int *p60_xmap;
static int p60_xmap_w;

static void p60_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P60_LW && (unsigned)yi < P60_LH) {
            float *p = p60_acc + (yi * P60_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

static void p60_color(const uint32_t *pal, float hue, float sat, float val,
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

static void p60_blit(uint32_t *fb, int w, int h)
{
    int i, x;
    int n = P60_LW * P60_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p60_acc[i] * 255.0f;
        p60_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p60_xmap_w != w) {
        free(p60_xmap);
        p60_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p60_xmap[x] = (int)(((long long)x * (P60_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p60_xmap_w = w;
    }
    jd_up_blit(&p060_up, fb, w, h, p60_img, P60_LW, P60_LH);
}

/* ------------- pattern state ------------- */
#define P60_NS 300
#define P60_NM 120
#define P60_LIVE 13
#define P60_MG 0.0035f
#define P60_MLIFE 265.0f

static int p60_init_done;
static float p60_sx[P60_NS], p60_sy[P60_NS], p60_sv[P60_NS];
static float p60_sp[P60_NS], p60_sh[P60_NS];
static float p60_mt[P60_NM], p60_mx0[P60_NM], p60_my0[P60_NM];
static float p60_mdx[P60_NM], p60_mvy[P60_NM], p60_mh[P60_NM];
static float p60_period;

static uint32_t p60_rs;
static float p60_rf(void)
{
    p60_rs ^= p60_rs << 13; p60_rs ^= p60_rs >> 17; p60_rs ^= p60_rs << 5;
    return (float)(p60_rs >> 8) * (1.0f / 16777216.0f);
}

static void p60_init(void)
{
    static const float hues[4] = {0.58f, 0.62f, 0.08f, 0.66f};
    int i;
    float acc = 0.0f;
    p60_rs = 0x60E7C012u;
    for (i = 0; i < P60_NS; i++) {
        p60_sx[i] = p60_rf() * P60_LW;
        p60_sy[i] = p60_rf() * P60_LH;
        p60_sv[i] = 0.10f + p60_rf() * 0.20f;
        p60_sp[i] = p60_rf() * P60_TAU;
        p60_sh[i] = hues[(int)(p60_rf() * 4.0f) & 3];
    }
    for (i = 0; i < P60_NM; i++) {
        acc += 34.0f + (float)((int)(p60_rf() * 28.0f));   /* 34..61 */
        p60_mt[i] = acc;
        p60_mx0[i] = 10.0f + p60_rf() * (P60_LW * 0.55f - 10.0f);
        p60_my0[i] = -16.0f + p60_rf() * 56.0f;
        p60_mdx[i] = 0.85f + p60_rf() * 0.45f;
        p60_mvy[i] = 0.45f + p60_rf() * 0.25f;
        p60_mh[i] = p60_rf();
    }
    p60_period = acc;
    p60_init_done = 1;
}

void pattern_060(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float tt, tc;
    int i, x, y, kbase, kk;
    (void)sl; (void)seed;
    if (!p60_init_done) p60_init();

    tt = (float)(frame % 16777216) + 160.0f;
    tc = fmodf(tt, p60_period);

    for (y = 0; y < P60_LH; y++) {
        float yy = (float)y / (float)(P60_LH - 1);
        float rr = 0.03f + 0.03f * yy;
        float gg = 0.035f * yy;
        float bb = 0.10f - 0.04f * yy;
        float *p = p60_acc + y * P60_LW * 3;
        for (x = 0; x < P60_LW; x++) {
            p[x * 3 + 0] = rr; p[x * 3 + 1] = gg; p[x * 3 + 2] = bb;
        }
    }
    for (i = 0; i < P60_NS; i++) {
        float tw = p60_sv[i] * (0.7f + 0.3f * sinf(tt * 0.03f + p60_sp[i])) * 1.6f;
        float cr, cg, cb;
        p60_color(pal, p60_sh[i], 0.35f, 1.0f, &cr, &cg, &cb);
        p60_splat(p60_sx[i], p60_sy[i], cr, cg, cb, tw);
    }

    /* newest launched meteor at the wrapped clock */
    kbase = -1;
    for (i = 0; i < P60_NM; i++) { if (p60_mt[i] < tc) kbase = i; else break; }

    for (kk = 0; kk < P60_LIVE; kk++) {
        int k = kbase - kk, ki, ns, j;
        float launch, age, amax, step, cr, cg, cb;
        float lx = 0.0f, ly = 0.0f;
        ki = k; launch = 0.0f;
        while (ki < 0) { ki += P60_NM; launch -= p60_period; }
        launch += p60_mt[ki];
        age = tc - launch;
        if (age <= 0.0f) continue;
        amax = age < P60_MLIFE ? age : P60_MLIFE;
        ns = (int)(amax / 1.5f) + 2;
        if (expf(-(age - amax) * (1.0f / 170.0f)) < 0.004f) continue;
        p60_color(pal, p60_mh[ki], 0.68f, 1.0f, &cr, &cg, &cb);
        step = amax / (float)(ns - 1);
        for (j = 0; j < ns; j++) {
            float a = step * (float)j;
            float ago = age - a;
            float wgt = expf(-ago * (1.0f / 170.0f))
                        * (0.30f + 0.70f * (a / (amax > 1.0f ? amax : 1.0f))) * 1.6f;
            lx = p60_mx0[ki] + p60_mdx[ki] * a;
            ly = p60_my0[ki] + p60_mvy[ki] * a + 0.5f * P60_MG * a * a;
            p60_splat(lx, ly, cr, cg, cb, wgt);
            p60_splat((float)(P60_LW - 1) - lx, ly, cr, cg, cb, wgt);
        }
        if (age < P60_MLIFE) {           /* bright bolide heads */
            p60_splat(lx, ly, 1.0f, 1.0f, 1.0f, 1.3f);
            p60_splat((float)(P60_LW - 1) - lx, ly, 1.0f, 1.0f, 1.0f, 1.3f);
        }
    }
    p60_blit(fb, w, h);
}
