/* 059 Binary Dance — waltzing binary stars with moons and a lemniscate stream.
 * Port of lab/patterns/059_binary_dance/proto.py.
 * Low-res float accumulator repainted every frame, bilinear upscale to fb. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
static jd_up p059_up;

#define P59_LW 320
#define P59_LH 240
#define P59_CX 160.0f
#define P59_CY 120.0f
#define P59_TAU 6.28318530717958647692f

static float p59_acc[P59_LW * P59_LH * 3];
static unsigned char p59_img[P59_LW * P59_LH * 3];
static int *p59_xmap;
static int p59_xmap_w;

static void p59_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P59_LW && (unsigned)yi < P59_LH) {
            float *p = p59_acc + (yi * P59_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

static void p59_color(const uint32_t *pal, float hue, float sat, float val,
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

static void p59_blit(uint32_t *fb, int w, int h)
{
    int i, x;
    int n = P59_LW * P59_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p59_acc[i] * 255.0f;
        p59_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p59_xmap_w != w) {
        free(p59_xmap);
        p59_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p59_xmap[x] = (int)(((long long)x * (P59_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p59_xmap_w = w;
    }
    jd_up_blit(&p059_up, fb, w, h, p59_img, P59_LW, P59_LH);
}

/* ------------- pattern state ------------- */
#define P59_NS 110
#define P59_OM 0.0045f
#define P59_R 46.0f
#define P59_HA 0.09f
#define P59_HB 0.55f
#define P59_TR 200
#define P59_MTR 70

static int p59_init_done;
static float p59_sx[P59_NS], p59_sy[P59_NS], p59_sp[P59_NS];
static float p59_fade[P59_TR], p59_fm[P59_MTR], p59_sat[P59_TR];

static uint32_t p59_rs;
static float p59_rf(void)
{
    p59_rs ^= p59_rs << 13; p59_rs ^= p59_rs >> 17; p59_rs ^= p59_rs << 5;
    return (float)(p59_rs >> 8) * (1.0f / 16777216.0f);
}

static void p59_init(void)
{
    int i;
    p59_rs = 0x59BD0CE3u;
    for (i = 0; i < P59_NS; i++) {
        p59_sx[i] = p59_rf() * P59_LW;
        p59_sy[i] = p59_rf() * P59_LH;
        p59_sp[i] = p59_rf() * P59_TAU;
    }
    for (i = 0; i < P59_TR; i++) {
        float u = 1.0f - (float)i / (float)P59_TR;
        p59_fade[i] = powf(u, 1.4f);
        p59_sat[i] = 0.85f - 0.6f * expf(-(float)i * (1.0f / 5.0f));
    }
    for (i = 0; i < P59_MTR; i++) {
        float u = 1.0f - (float)i / (float)P59_MTR;
        p59_fm[i] = powf(u, 1.5f);
    }
    p59_init_done = 1;
}

static void p59_star(float times, float side, float *x, float *y)
{
    float a = times * P59_OM + side;
    *x = P59_CX + P59_R * cosf(a);
    *y = P59_CY + P59_R * sinf(a) * 0.78f;
}

void pattern_059(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float tt = (float)(frame % 1048576) + 400.0f;
    int i, j, m, side_i, n;
    (void)sl; (void)seed;
    if (!p59_init_done) p59_init();

    n = P59_LW * P59_LH;
    for (i = 0; i < n; i++) {
        p59_acc[i * 3 + 0] = 0.025f;
        p59_acc[i * 3 + 1] = 0.0f;
        p59_acc[i * 3 + 2] = 0.055f;
    }
    for (i = 0; i < P59_NS; i++) {
        float tv = 0.08f + 0.06f * sinf(tt * 0.035f + p59_sp[i]);
        p59_splat(p59_sx[i], p59_sy[i], tv, tv, tv * 1.2f, 1.0f);
    }

    for (side_i = 0; side_i < 2; side_i++) {
        float side = side_i ? 3.14159265358979f : 0.0f;
        float hb = side_i ? P59_HB : P59_HA;
        float hx = 0.0f, hy = 0.0f;
        /* star waltz trail */
        for (j = 0; j < P59_TR; j++) {
            float x, y, cr, cg, cb;
            p59_star(tt - (float)j * 3.2f, side, &x, &y);
            p59_color(pal, hb, p59_sat[j], 1.0f, &cr, &cg, &cb);
            p59_splat(x, y, cr, cg, cb, p59_fade[j] * 1.15f);
            if (!j) { hx = x; hy = y; }
        }
        /* moons: 4 per star with short trails */
        for (m = 0; m < 4; m++) {
            float rm = 13.0f + 4.8f * (float)m;
            float omm = (0.052f - 0.007f * (float)m) * ((m & 1) ? -1.0f : 1.0f);
            float cr, cg, cb;
            p59_color(pal, hb + 0.06f * ((float)m - 1.5f), 0.8f, 1.0f, &cr, &cg, &cb);
            for (j = 0; j < P59_MTR; j++) {
                float tm = tt - (float)j;
                float sx, sy, ma;
                p59_star(tm, side, &sx, &sy);
                ma = omm * tm + (float)m * 1.9f + side;
                p59_splat(sx + rm * cosf(ma), sy + rm * sinf(ma) * 0.88f,
                          cr, cg, cb, p59_fm[j] * 0.85f);
            }
        }
        /* star core + two halo dot rings */
        p59_splat(hx, hy, 1.0f, 1.0f, 1.0f, 1.6f);
        {
            float cr, cg, cb, rr;
            int ri;
            p59_color(pal, hb, 0.5f, 1.0f, &cr, &cg, &cb);
            for (ri = 0; ri < 2; ri++) {
                rr = ri ? 6.0f : 3.0f;
                for (j = 0; j < 18; j++) {
                    float a = (float)j * (P59_TAU / 18.0f);
                    p59_splat(hx + rr * cosf(a), hy + rr * sinf(a) * 0.85f,
                              cr, cg, cb, 0.55f);
                }
            }
        }
    }

    /* exchange stream: figure-8 lemniscate rotating with the pair */
    {
        float rot = tt * P59_OM;
        float crr = cosf(rot), srr = sinf(rot);
        float L = P59_R * 2.05f;
        for (i = 0; i < 220; i++) {
            float s = tt * 0.008f + (float)i * (P59_TAU / 110.0f);
            float ss = sinf(s), cs = cosf(s);
            float d = 1.0f + ss * ss;
            float lx = L * cs / d;
            float ly = L * ss * cs / d * 1.35f;
            float x = P59_CX + lx * crr - ly * srr;
            float y = P59_CY + (lx * srr + ly * crr) * 0.78f;
            float hue = P59_HA + (P59_HB - P59_HA) * (0.5f - 0.5f * cs);
            float cr, cg, cb;
            p59_color(pal, hue, 0.7f, 1.0f, &cr, &cg, &cb);
            p59_splat(x, y, cr, cg, cb, 0.55f);
        }
    }
    p59_blit(fb, w, h);
}
