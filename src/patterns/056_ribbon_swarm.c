/* 056 Ribbon Swarm — a Lissajous murmuration folding and unfolding, mirrored.
 * Port of lab/patterns/056_ribbon_swarm/proto.py.
 * Low-res float accumulator repainted every frame, bilinear upscale to fb. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
static jd_up p056_up;

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
    int i, x;
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
    jd_up_blit(&p056_up, fb, w, h, p56_img, P56_LW, P56_LH);
}

/* ------------- pattern state ------------- */
#define P56_N 900
#define P56_NTR 9

static int p56_init_done;
static float p56_hu[P56_N];     /* u = i/N */
static float p56_fade[P56_NTR];

/* 3.6.5: per-appearance figure — the seed used to be ignored, so every
 * appearance traced the same 2:3 ribbon (variety audit). The seed now picks
 * the Lissajous pair, the ribbon's width/height and spread, which way it
 * turns, a start phase, and where on the ramp the hues start.
 * The strand's y:x phase ratio along u MUST equal the y:x time-rate ratio
 * (it was 1.5 against 2:3): then the figure holds still and the strand slides
 * along it. Mismatch them and the whole knot churns about 7x faster (tried). */
static uint32_t p56_seed = 0xFFFFFFFFu;
static float p56_fa = 2.0f, p56_fb = 3.0f, p56_ax = 118.0f, p56_ay = 86.0f;
static float p56_sp0 = 1.7f, p56_dir = 1.0f, p56_th0, p56_h0;
static uint32_t p56_h(uint32_t x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }

static void p56_reseed(uint32_t seed)
{
    static const float fa[6] = {2.0f, 3.0f, 2.0f, 4.0f, 3.0f, 1.0f};
    static const float fb[6] = {3.0f, 4.0f, 5.0f, 5.0f, 5.0f, 2.0f};
    uint32_t a = p56_h(seed ^ 0x5656u), b = p56_h(a), c = p56_h(b);
    int k = (int)(a % 6u);
    p56_fa  = fa[k]; p56_fb = fb[k];
    p56_ax  = 106.0f + (float)((a >> 8) % 1000u) * 0.024f;        /* 106..130 */
    p56_ay  = 76.0f + (float)((a >> 18) % 1000u) * 0.020f;        /* 76..96 */
    p56_sp0 = 1.4f + (float)((b >> 4) % 1000u) * 0.0006f;         /* 1.4..2.0 */
    p56_dir = (b & 1u) ? 1.0f : -1.0f;
    p56_th0 = (float)((b >> 16) % 1000u) * (P56_TAU / 1000.0f);
    p56_h0  = (float)((c >> 6) % 1000u) * 0.001f;
    p56_seed = seed;
}

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
    (void)sl;
    if (!p56_init_done) p56_init();
    if (seed != p56_seed) p56_reseed(seed);

    n = P56_LW * P56_LH;
    for (i = 0; i < n; i++) {
        p56_acc[i * 3 + 0] = 0.03f;
        p56_acc[i * 3 + 1] = 0.0f;
        p56_acc[i * 3 + 2] = 0.07f;
    }

    spread = p56_sp0 + 1.1f * sinf(tt * 0.0008f);

    for (j = 0; j < P56_NTR; j++) {
        float ttj = tt - (float)j * 5.2f;
        /* x phase advances at the old 2 x 0.004 whatever the pair */
        float th = ttj * (0.0080f / p56_fa) * p56_dir + p56_th0;
        float drift = ttj * 0.0013f;
        float a2 = p56_fa * th, a3 = p56_fb * th + drift;
        float kr = p56_fb / p56_fa;
        float fade = p56_fade[j];
        float sat = j ? 0.92f : 0.5f;
        float val = 1.0f - 0.30f * ((float)j / (float)P56_NTR);
        float wf = fade * 0.85f, wb = fade * 0.28f;
        float hue0 = p56_h0 + tt * 0.00025f;
        for (i = 0; i < P56_N; i++) {
            float u = p56_hu[i];
            float ph = u * P56_TAU * spread;
            float x0 = p56_ax * sinf(a2 + ph);
            float y0 = p56_ay * sinf(a3 + ph * kr);
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
