/* 053 Gravity Rose — precessing Kepler petals, 4-fold mirrored comet trails.
 * Port of lab/patterns/053_gravity_rose/proto.py. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P53_LW 320
#define P53_LH 240
#define P53_CX 160.0f
#define P53_CY 120.0f
#define P53_TAU 6.28318530717958647692f

static float p53_acc[P53_LW * P53_LH * 3];
static float p53_gnd[P53_LW * P53_LH * 3];
static unsigned char p53_img[P53_LW * P53_LH * 3];
static int *p53_xmap;
static int p53_xmap_w;

static void p53_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P53_LW && (unsigned)yi < P53_LH) {
            float *p = p53_acc + (yi * P53_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

static void p53_color(const uint32_t *pal, float hue, float sat, float val,
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

static void p53_blit(uint32_t *fb, int w, int h)
{
    int i, x, y;
    int n = P53_LW * P53_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p53_acc[i] * 255.0f;
        p53_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p53_xmap_w != w) {
        free(p53_xmap);
        p53_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p53_xmap[x] = (int)(((long long)x * (P53_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p53_xmap_w = w;
    }
    for (y = 0; y < h; y++) {
        int sy = (int)(((long long)y * (P53_LH - 1) << 8) / (h > 1 ? h - 1 : 1));
        int y0 = sy >> 8, fy = sy & 255;
        int y1 = y0 + 1 < P53_LH ? y0 + 1 : P53_LH - 1;
        const unsigned char *r0 = p53_img + y0 * P53_LW * 3;
        const unsigned char *r1 = p53_img + y1 * P53_LW * 3;
        uint32_t *dst = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int sx = p53_xmap[x];
            int x0 = sx >> 8, fx = sx & 255;
            int x1 = x0 + 1 < P53_LW ? x0 + 1 : P53_LW - 1;
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

/* ------------- pattern ------------- */
#define P53_NPART 8
#define P53_TRAIL 180
static int p53_init_done;

static void p53_init(void)
{
    int x, y;
    for (y = 0; y < P53_LH; y++)
        for (x = 0; x < P53_LW; x++) {
            float dx = (float)x - P53_CX, dy = ((float)y - P53_CY) * 1.15f;
            float d = sqrtf(dx * dx + dy * dy);
            float d13 = d / 13.0f, d11 = d / 11.0f;
            float *p = p53_gnd + (y * P53_LW + x) * 3;
            p[0] = 0.02f + 0.26f * expf(-d13 * d13);
            p[1] = 0.18f * expf(-d11 * d11);
            p[2] = 0.06f + 0.05f * expf(-d / 90.0f);
        }
    p53_init_done = 1;
}

void pattern_053(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float tt = (float)(frame % 1048576) + 260.0f;
    int i, j, p;
    (void)sl; (void)seed;
    if (!p53_init_done) p53_init();

    for (i = 0; i < P53_LW * P53_LH * 3; i++)
        p53_acc[i] = p53_gnd[i];

    for (p = 0; p < P53_NPART; p++) {
        float om = 0.011f + 0.0016f * (float)p;
        float prec = 0.0021f * (1.0f + 0.35f * (float)p);
        float e = 0.38f;
        float ap = 40.0f + 6.5f * (float)p;
        float hue = (float)p / (float)P53_NPART + tt * 0.00045f;
        for (j = 0; j < P53_TRAIL; j++) {
            float times = tt - (float)j * 1.35f;
            float fj = 1.0f - (float)j / (float)P53_TRAIL;
            float fade = fj * sqrtf(fj);           /* ^1.5 */
            float phi = om * times + (float)p * 2.399f;
            float rel = phi - prec * times;
            float rr = ap * (1.0f - e * e) / (1.0f + e * cosf(rel));
            float x = rr * cosf(phi) * 0.98f;
            float y = rr * sinf(phi) * 0.80f;
            float sat = 0.85f - 0.55f * expf(-(float)j / 6.0f);
            float val = 1.0f - 0.35f * ((float)j / (float)P53_TRAIL);
            float cr, cg, cb, wgt = fade * 0.95f;
            p53_color(pal, hue, sat, val, &cr, &cg, &cb);
            p53_splat(P53_CX + x, P53_CY + y, cr, cg, cb, wgt);
            p53_splat(P53_CX - x, P53_CY + y, cr, cg, cb, wgt);
            p53_splat(P53_CX + x, P53_CY - y, cr, cg, cb, wgt);
            p53_splat(P53_CX - x, P53_CY - y, cr, cg, cb, wgt);
        }
    }
    p53_blit(fb, w, h);
}
