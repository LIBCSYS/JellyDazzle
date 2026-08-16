/* 055 Comet Carousel — three comets, six-fold kaleidoscope, rotating tail pinwheel.
 * Port of lab/patterns/055_comet_carousel/proto.py.
 * Low-res float accumulator repainted every frame, bilinear upscale to fb. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
static jd_up p055_up;

#define P55_LW 320
#define P55_LH 240
#define P55_CX 160.0f
#define P55_CY 120.0f
#define P55_TAU 6.28318530717958647692f

static float p55_acc[P55_LW * P55_LH * 3];
static unsigned char p55_img[P55_LW * P55_LH * 3];
static int *p55_xmap;
static int p55_xmap_w;

static void p55_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P55_LW && (unsigned)yi < P55_LH) {
            float *p = p55_acc + (yi * P55_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

static void p55_color(const uint32_t *pal, float hue, float sat, float val,
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

static void p55_blit(uint32_t *fb, int w, int h)
{
    int i, x;
    int n = P55_LW * P55_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p55_acc[i] * 255.0f;
        p55_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p55_xmap_w != w) {
        free(p55_xmap);
        p55_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p55_xmap[x] = (int)(((long long)x * (P55_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p55_xmap_w = w;
    }
    jd_up_blit(&p055_up, fb, w, h, p55_img, P55_LW, P55_LH);
}

/* ------------- pattern state ------------- */
#define P55_TRAIL 180
#define P55_NC 3

static int p55_init_done;
static float p55_bgr[P55_LW * P55_LH];   /* red ground */
static float p55_bgb[P55_LW * P55_LH];   /* blue ground */
static float p55_fade[P55_TRAIL];

static void p55_init(void)
{
    int x, y, j;
    for (y = 0; y < P55_LH; y++) {
        float dy = (float)y - P55_CY;
        for (x = 0; x < P55_LW; x++) {
            float dx = (float)x - P55_CX;
            float d = sqrtf(dx * dx + dy * dy);
            int o = y * P55_LW + x;
            p55_bgb[o] = 0.055f + 0.06f * expf(-d * (1.0f / 100.0f));
            p55_bgr[o] = 0.035f * expf(-d * (1.0f / 70.0f));
        }
    }
    for (j = 0; j < P55_TRAIL; j++) {
        float u = 1.0f - (float)j / (float)P55_TRAIL;
        p55_fade[j] = powf(u, 1.4f);
    }
    p55_init_done = 1;
}

void pattern_055(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float tt = (float)(frame % 1048576) + 300.0f;
    float grot, mca[6], msa[6];
    int i, j, c, m, ring, n;
    (void)sl; (void)seed;
    if (!p55_init_done) p55_init();

    n = P55_LW * P55_LH;
    for (i = 0; i < n; i++) {
        p55_acc[i * 3 + 0] = p55_bgr[i];
        p55_acc[i * 3 + 1] = 0.0f;
        p55_acc[i * 3 + 2] = p55_bgb[i];
    }

    grot = tt * 0.0021f;
    for (m = 0; m < 6; m++) {
        float a = grot + (float)m * (P55_TAU / 6.0f);
        mca[m] = cosf(a); msa[m] = sinf(a);
    }

    for (c = 0; c < P55_NC; c++) {
        float om = 0.016f + 0.0055f * (float)c;
        float hue0 = (float)c * 0.31f + tt * 0.0006f;
        for (j = 0; j < P55_TRAIL; j++) {
            float times = tt - (float)j * 1.5f;
            float u = (float)j / (float)P55_TRAIL;
            float th = times * om + (float)c * 2.1f;
            float rad = 38.0f + 20.0f * (float)c
                        + 30.0f * sinf(times * 0.0075f + (float)c * 1.7f);
            float x0 = rad * cosf(th);
            float y0 = rad * sinf(th) * 0.85f;
            float sat = 0.85f - 0.6f * expf(-(float)j * (1.0f / 6.0f));
            float val = 1.0f - 0.30f * u;
            float wgt = p55_fade[j] * 1.05f;
            float cr, cg, cb;
            p55_color(pal, hue0 + 0.11f * u, sat, val, &cr, &cg, &cb);
            for (m = 0; m < 6; m++) {
                float ca = mca[m], sa = msa[m];
                float x = P55_CX + x0 * ca - y0 * sa;
                float y = P55_CY + (x0 * sa + y0 * ca) * 0.92f;
                p55_splat(x, y, cr, cg, cb, wgt);
            }
        }
    }

    /* center medallion: two counter-rotating dot rings */
    for (ring = 0; ring < 2; ring++) {
        float rr = 7.0f + 6.0f * (float)ring + 2.0f * sinf(tt * 0.02f + (float)ring * 2.0f);
        int ns = 12 + 8 * ring;
        float dir = ring ? 1.0f : -1.0f;
        float cr, cg, cb;
        p55_color(pal, 0.12f + tt * 0.0006f + 0.3f * (float)ring, 0.6f, 1.0f, &cr, &cg, &cb);
        for (j = 0; j < ns; j++) {
            float aa = (float)j * (P55_TAU / (float)ns) + tt * 0.015f * dir;
            p55_splat(P55_CX + rr * cosf(aa), P55_CY + rr * sinf(aa) * 0.92f,
                      cr, cg, cb, 0.85f);
        }
    }
    p55_blit(fb, w, h);
}
