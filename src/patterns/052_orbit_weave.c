/* 052 Orbit Weave — precessing electron ellipses tracing a woven rosette.
 * Port of lab/patterns/052_orbit_weave/proto.py.
 * Low-res float accumulator repainted every frame, bilinear upscale to fb. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
static jd_up p052_up;

#define P52_LW 320
#define P52_LH 240
#define P52_CX 160.0f
#define P52_CY 120.0f
#define P52_TAU 6.28318530717958647692f

static float p52_acc[P52_LW * P52_LH * 3];
static unsigned char p52_img[P52_LW * P52_LH * 3];
static int *p52_xmap;
static int p52_xmap_w;

static void p52_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P52_LW && (unsigned)yi < P52_LH) {
            float *p = p52_acc + (yi * P52_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

/* hue-family color: HSV chroma blended 35% with a saturated palette sample */
static void p52_color(const uint32_t *pal, float hue, float sat, float val,
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

static void p52_blit(uint32_t *fb, int w, int h)
{
    int i, x;
    int n = P52_LW * P52_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p52_acc[i] * 255.0f;
        p52_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p52_xmap_w != w) {
        free(p52_xmap);
        p52_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p52_xmap[x] = (int)(((long long)x * (P52_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p52_xmap_w = w;
    }
    jd_up_blit(&p052_up, fb, w, h, p52_img, P52_LW, P52_LH);
}

/* ------------- pattern state ------------- */
#define P52_NS 130
#define P52_M 230
static int p52_init_done;
static float p52_sx[P52_NS], p52_sy[P52_NS], p52_sp[P52_NS];

static uint32_t p52_rs;
static float p52_rf(void)
{
    p52_rs ^= p52_rs << 13; p52_rs ^= p52_rs >> 17; p52_rs ^= p52_rs << 5;
    return (float)(p52_rs >> 8) * (1.0f / 16777216.0f);
}

static void p52_init(void)
{
    int i;
    p52_rs = 0x520AB1Du;
    for (i = 0; i < P52_NS; i++) {
        p52_sx[i] = p52_rf() * P52_LW;
        p52_sy[i] = p52_rf() * P52_LH;
        p52_sp[i] = p52_rf() * P52_TAU;
    }
    p52_init_done = 1;
}

void pattern_052(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float tt = (float)(frame % 1048576) + 60.0f;
    int i, j, ring, n;
    (void)sl; (void)seed;
    if (!p52_init_done) p52_init();

    n = P52_LW * P52_LH;
    for (i = 0; i < n; i++) {
        p52_acc[i * 3 + 0] = 0.015f;
        p52_acc[i * 3 + 1] = 0.0f;
        p52_acc[i * 3 + 2] = 0.05f;
    }
    /* dust stars twinkling faintly */
    for (i = 0; i < P52_NS; i++) {
        float tv = 0.10f + 0.08f * sinf(tt * 0.04f + p52_sp[i]);
        p52_splat(p52_sx[i], p52_sy[i], tv, tv, tv * 1.3f, 1.0f);
    }
    for (i = 0; i < 7; i++) {
        float prec = 0.0016f + 0.00045f * (float)i;
        float th = (float)i * (P52_TAU / 14.0f) + tt * prec;
        float A = 128.0f - 9.0f * (float)i;
        float B = 26.0f + 11.0f * (float)i;
        float om = 0.020f + 0.004f * (float)i;
        float ph = tt * om + (float)i * 1.3f;
        float c = cosf(th), sn = sinf(th);
        float hue = 0.52f + 0.085f * (float)i + tt * 0.0007f;
        for (j = 0; j < P52_M; j++) {
            float s = ph - 2.6f * (float)j / (float)(P52_M - 1);
            float ex = A * cosf(s), ey = B * sinf(s);
            float x = P52_CX + ex * c - ey * sn;
            float y = P52_CY + (ex * sn + ey * c) * 0.92f;
            float fb1 = 1.0f - 0.95f * (float)j / (float)(P52_M - 1);
            float fade = fb1 * sqrtf(fb1);          /* ^1.5 */
            float sat = 0.85f - 0.55f * expf(-(float)j / 7.0f);
            float cr, cg, cb;
            p52_color(pal, hue, sat, 1.0f, &cr, &cg, &cb);
            p52_splat(x, y, cr, cg, cb, fade * 0.9f);
            p52_splat(2.0f * P52_CX - x, 2.0f * P52_CY - y, cr, cg, cb, fade * 0.9f);
        }
        /* faint dotted full orbit ring */
        {
            float cr, cg, cb;
            p52_color(pal, hue, 0.9f, 1.0f, &cr, &cg, &cb);
            for (j = 0; j < 110; j++) {
                float sf = (float)j * (P52_TAU / 110.0f) + tt * 0.002f;
                float ex = A * cosf(sf), ey = B * sinf(sf);
                float x = P52_CX + ex * c - ey * sn;
                float y = P52_CY + (ex * sn + ey * c) * 0.92f;
                p52_splat(x, y, cr, cg, cb, 0.085f);
            }
        }
    }
    /* pulsing nucleus of dot rings */
    for (ring = 0; ring < 3; ring++) {
        float rr = 4.0f + 3.6f * (float)ring + 1.6f * sinf(tt * 0.03f + (float)ring * 1.1f);
        int ns = 10 + 6 * ring;
        float dir = (ring % 2) ? 1.0f : -1.0f;
        float cr, cg, cb;
        p52_color(pal, 0.11f + tt * 0.0007f, 0.55f, 1.0f, &cr, &cg, &cb);
        for (j = 0; j < ns; j++) {
            float aa = (float)j * (P52_TAU / (float)ns) + tt * 0.012f * dir;
            p52_splat(P52_CX + rr * cosf(aa), P52_CY + rr * sinf(aa), cr, cg, cb, 0.9f);
        }
    }
    p52_blit(fb, w, h);
}
