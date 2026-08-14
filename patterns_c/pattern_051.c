/* 051 Firework Garden — accumulating gravity-drooped bursts on a plum ground.
 * Port of lab/patterns/051_firework_garden/proto.py.
 * Renders into a 320x240 float accumulator, bilinear-upscales to fb.
 * Full repaint every call (ground repainted, bursts derived from sl+seed). */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
static jd_up p051_up;

#define P51_LW 320
#define P51_LH 240
#define P51_TAU 6.28318530717958647692f

static float p51_acc[P51_LW * P51_LH * 3];
static unsigned char p51_img[P51_LW * P51_LH * 3];
static int *p51_xmap;
static int p51_xmap_w;

static void p51_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P51_LW && (unsigned)yi < P51_LH) {
            float *p = p51_acc + (yi * P51_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

/* fetch palette color at hue position, blend toward white by (1-sat), scale v */
static void p51_palrgb(const uint32_t *pal, float hue, float sat, float val,
                       float *r, float *g, float *b)
{
    uint32_t p;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    *r = val * ((1.0f - sat) + sat * (float)((p >> 16) & 255) * (1.0f / 255.0f));
    *g = val * ((1.0f - sat) + sat * (float)((p >> 8) & 255) * (1.0f / 255.0f));
    *b = val * ((1.0f - sat) + sat * (float)(p & 255) * (1.0f / 255.0f));
}

/* vivid fetch: map hue into the saturated palette band, normalize to full value */
static void p51_vivid(const uint32_t *pal, float hue, float sat,
                      float *r, float *g, float *b)
{
    uint32_t p;
    float fr, fg, fb2, mx;
    hue -= floorf(hue);
    p = pal[(int)(hue * 0.55f * 32767.0f) & JD_PAL_MASK];
    fr = (float)((p >> 16) & 255); fg = (float)((p >> 8) & 255); fb2 = (float)(p & 255);
    mx = fr > fg ? fr : fg; if (fb2 > mx) mx = fb2;
    if (mx < 1.0f) mx = 1.0f;
    fr /= mx; fg /= mx; fb2 /= mx;
    *r = (1.0f - sat) + sat * fr;
    *g = (1.0f - sat) + sat * fg;
    *b = (1.0f - sat) + sat * fb2;
}

static void p51_blit(uint32_t *fb, int w, int h)
{
    int i, x;
    int n = P51_LW * P51_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p51_acc[i] * 255.0f;
        p51_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p51_xmap_w != w) {
        free(p51_xmap);
        p51_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p51_xmap[x] = (int)(((long long)x * (P51_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p51_xmap_w = w;
    }
    jd_up_blit(&p051_up, fb, w, h, p51_img, P51_LW, P51_LH);
}

/* ------------- pattern state ------------- */
#define P51_NB 64
#define P51_NP 44
#define P51_G 0.0105f
#define P51_LIFE 95.0f

static uint32_t p51_seed_cache;
static int p51_init_done;
static float p51_bt[P51_NB], p51_bx[P51_NB], p51_by[P51_NB], p51_bh[P51_NB];
static float p51_ca[P51_NB][P51_NP], p51_sa[P51_NB][P51_NP];
static float p51_sp[P51_NB][P51_NP], p51_s3[P51_NB][P51_NP];

static uint32_t p51_rs;
static float p51_rf(void)
{
    p51_rs ^= p51_rs << 13; p51_rs ^= p51_rs >> 17; p51_rs ^= p51_rs << 5;
    return (float)(p51_rs >> 8) * (1.0f / 16777216.0f);
}

static void p51_init(uint32_t seed)
{
    int k, i;
    float t = 0.0f;
    p51_rs = seed ? seed : 0x51F1DEu;
    for (k = 0; k < P51_NB; k++) {
        t += 24.0f + p51_rf() * 20.0f;
        p51_bt[k] = t;
        p51_bx[k] = 34.0f + p51_rf() * (P51_LW - 68.0f);
        p51_by[k] = 26.0f + p51_rf() * (P51_LH * 0.60f - 26.0f);
        p51_bh[k] = p51_rf();
        for (i = 0; i < P51_NP; i++) {
            float ang = p51_rf() * P51_TAU;
            float s3 = sinf(ang * 3.0f);
            p51_ca[k][i] = cosf(ang);
            p51_sa[k][i] = sinf(ang);
            p51_s3[k][i] = s3;
            p51_sp[k][i] = (0.5f + p51_rf() * 0.95f) * (1.0f + 0.15f * s3);
        }
    }
    p51_seed_cache = seed;
    p51_init_done = 1;
}

void pattern_051(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float tt = (float)sl + 110.0f;
    int y, x, k, i, si, nlive, first;
    float gr, gg, gb;
    (void)frame;
    if (!p51_init_done || p51_seed_cache != seed)
        p51_init(seed);

    /* plum -> indigo vertical ground from a dark violet palette entry */
    p51_palrgb(pal, 0.105f, 1.0f, 1.0f, &gr, &gg, &gb);
    for (y = 0; y < P51_LH; y++) {
        float yy = (float)y / (float)(P51_LH - 1);
        float fr = 0.30f + 0.24f * yy;
        float *row = p51_acc + y * P51_LW * 3;
        float cr = gr * fr * 0.9f + 0.02f, cg = gg * fr * 0.5f, cb = gb * fr + 0.03f;
        for (x = 0; x < P51_LW; x++) {
            row[x * 3 + 0] = cr; row[x * 3 + 1] = cg; row[x * 3 + 2] = cb;
        }
    }

    nlive = 0;
    for (k = 0; k < P51_NB; k++)
        if (p51_bt[k] < tt) nlive++;
    first = nlive > 20 ? nlive - 20 : 0;
    for (k = first; k < nlive; k++) {
        float age = tt - p51_bt[k];
        float amax = age < P51_LIFE ? age : P51_LIFE;
        int ns = (int)amax + 2;
        float step = amax / (float)(ns - 1);
        float over = age - P51_LIFE;
        float old = over > 0.0f ? expf(-over / 150.0f) : 1.0f;
        float amx = amax > 1.0f ? amax : 1.0f;
        float pr[P51_NP], pg[P51_NP], pb[P51_NP];
        for (i = 0; i < P51_NP; i++)
            p51_vivid(pal, p51_bh[k] + 0.05f * p51_s3[k][i], 0.92f,
                      &pr[i], &pg[i], &pb[i]);
        for (si = 0; si < ns; si++) {
            float a = step * (float)si;
            float fade = powf(a / amx, 1.7f);
            float wgt = (0.15f + 0.85f * fade) * old * 0.9f;
            float drop = 0.5f * P51_G * a * a;
            for (i = 0; i < P51_NP; i++) {
                float d = p51_sp[k][i] * a;
                p51_splat(p51_bx[k] + p51_ca[k][i] * d,
                          p51_by[k] + p51_sa[k][i] * d * 0.85f + drop,
                          pr[i], pg[i], pb[i], wgt);
            }
        }
        if (age < P51_LIFE) {         /* hot white heads */
            float drop = 0.5f * P51_G * amax * amax;
            for (i = 0; i < P51_NP; i++) {
                float d = p51_sp[k][i] * amax;
                p51_splat(p51_bx[k] + p51_ca[k][i] * d,
                          p51_by[k] + p51_sa[k][i] * d * 0.85f + drop,
                          1.0f, 1.0f, 1.0f, 0.8f);
            }
        }
    }
    p51_blit(fb, w, h);
}
