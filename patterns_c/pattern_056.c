/* 056 Ribbon Swarm — a Lissajous murmuration folding and unfolding, mirrored.
 * Port of lab/patterns/056_ribbon_swarm/proto.py.
 * Low-res float accumulator repainted every frame, bilinear upscale to fb. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P56_LW 320
#define P56_LH 240
#define P56_CX 160.0f
#define P56_CY 120.0f
#define P56_TAU 6.28318530717958647692f

static float p56_acc[P56_LW * P56_LH * 3];
static unsigned char p56_img[P56_LW * P56_LH * 3];
static int *p56_xmap;
static int p56_xmap_w;

static void p56_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P56_LW && (unsigned)yi < P56_LH) {
            float *p = p56_acc + (yi * P56_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

static void p56_color(const uint32_t *pal, float hue, float sat, float val,
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

static void p56_blit(uint32_t *fb, int w, int h)
{
    int i, x, y;
    int n = P56_LW * P56_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p56_acc[i] * 255.0f;
        p56_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p56_xmap_w != w) {
        free(p56_xmap);
        p56_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p56_xmap[x] = (int)(((long long)x * (P56_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p56_xmap_w = w;
    }
    for (y = 0; y < h; y++) {
        int sy = (int)(((long long)y * (P56_LH - 1) << 8) / (h > 1 ? h - 1 : 1));
        int y0 = sy >> 8, fy = sy & 255;
        int y1 = y0 + 1 < P56_LH ? y0 + 1 : P56_LH - 1;
        const unsigned char *r0 = p56_img + y0 * P56_LW * 3;
        const unsigned char *r1 = p56_img + y1 * P56_LW * 3;
        uint32_t *dst = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int sx = p56_xmap[x];
            int x0 = sx >> 8, fx = sx & 255;
            int x1 = x0 + 1 < P56_LW ? x0 + 1 : P56_LW - 1;
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
#define P56_N 300
#define P56_NTR 9

static int p56_init_done;
static float p56_hu[P56_N];     /* u = i/N */
static float p56_fade[P56_NTR];

static void p56_init(void)
{
    int i;
    for (i = 0; i < P56_N; i++) p56_hu[i] = (float)i / (float)P56_N;
    for (i = 0; i < P56_NTR; i++) {
        float u = 1.0f - (float)i / (float)P56_NTR;
        p56_fade[i] = powf(u, 1.4f);
    }
    p56_init_done = 1;
}

void pattern_056(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float tt = (float)(frame % 1048576) + 150.0f;
    float spread;
    int i, j, n;
    (void)sl; (void)seed;
    if (!p56_init_done) p56_init();

    n = P56_LW * P56_LH;
    for (i = 0; i < n; i++) {
        p56_acc[i * 3 + 0] = 0.03f;
        p56_acc[i * 3 + 1] = 0.0f;
        p56_acc[i * 3 + 2] = 0.07f;
    }

    spread = 1.7f + 1.1f * sinf(tt * 0.0019f);

    for (j = 0; j < P56_NTR; j++) {
        float ttj = tt - (float)j * 2.2f;
        float th = ttj * 0.0095f;
        float drift = ttj * 0.0031f;
        float a2 = 2.0f * th, a3 = 3.0f * th + drift;
        float fade = p56_fade[j];
        float sat = j ? 0.92f : 0.5f;
        float val = 1.0f - 0.30f * ((float)j / (float)P56_NTR);
        float wf = fade * 0.85f, wb = fade * 0.28f;
        float hue0 = tt * 0.0006f;
        for (i = 0; i < P56_N; i++) {
            float u = p56_hu[i];
            float ph = u * P56_TAU * spread;
            float x0 = 118.0f * sinf(a2 + ph);
            float y0 = 86.0f * sinf(a3 + ph * 1.5f);
            float cr, cg, cb;
            p56_color(pal, hue0 + u * 0.9f, sat, val, &cr, &cg, &cb);
            p56_splat(P56_CX + x0, P56_CY + y0, cr, cg, cb, wf);
            p56_splat(P56_CX - x0, P56_CY + y0, cr, cg, cb, wf);
            p56_splat(P56_CX + x0, P56_CY - y0, cr, cg, cb, wb);
            p56_splat(P56_CX - x0, P56_CY - y0, cr, cg, cb, wb);
        }
    }
    p56_blit(fb, w, h);
}
