/* 058 Fountain Arcs — mirrored ballistic jets arcing from a warm emitter.
 * Port of lab/patterns/058_fountain_arcs/proto.py.
 * Low-res float accumulator repainted every frame, bilinear upscale to fb. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
static jd_up p058_up;

#define P58_LW 320
#define P58_LH 240
#define P58_CX 160.0f
#define P58_CY 120.0f
#define P58_TAU 6.28318530717958647692f

static float p58_acc[P58_LW * P58_LH * 3];
static unsigned char p58_img[P58_LW * P58_LH * 3];
static int *p58_xmap;
static int p58_xmap_w;

static void p58_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P58_LW && (unsigned)yi < P58_LH) {
            float *p = p58_acc + (yi * P58_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

static void p58_color(const uint32_t *pal, float hue, float sat, float val,
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
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];          /* 3.6.5: the whole ramp */
    vr = (float)((p >> 16) & 255); vg = (float)((p >> 8) & 255); vb = (float)(p & 255);
    mx = vr > vg ? vr : vg; if (vb > mx) mx = vb;
    if (mx < 1.0f) mx = 1.0f;
    vr /= mx; vg /= mx; vb /= mx;
    /* 3.6.5 (J: "I see this fountain too much"): colour was 65% a fixed HSV
     * rainbow, so C barely changed it and every appearance looked the same.
     * Now it is the palette; the rainbow only keeps 10% for sparkle. */
    hr = 0.10f * hr + 0.90f * vr;
    hg = 0.10f * hg + 0.90f * vg;
    hb = 0.10f * hb + 0.90f * vb;
    *r = val * ((1.0f - sat) + sat * hr);
    *g = val * ((1.0f - sat) + sat * hg);
    *b = val * ((1.0f - sat) + sat * hb);
}

static void p58_blit(uint32_t *fb, int w, int h)
{
    int i, x;
    int n = P58_LW * P58_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p58_acc[i] * 255.0f;
        p58_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p58_xmap_w != w) {
        free(p58_xmap);
        p58_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p58_xmap[x] = (int)(((long long)x * (P58_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p58_xmap_w = w;
    }
    jd_up_blit(&p058_up, fb, w, h, p58_img, P58_LW, P58_LH);
}

/* ------------- pattern state ------------- */
#define P58_NL 1200          /* jet parameter cycle length */
#define P58_LIVE 120         /* most recent jets kept alive */
#define P58_G 0.030f
#define P58_BY (P58_LH - 14.0f)

static int p58_init_done;
/* 3.6.5: per-appearance shape (the seed used to be ignored) */
static uint32_t p58_seed = 0xFFFFFFFFu;
static float p58_cx = P58_CX, p58_spread = 1.0f, p58_pow = 1.0f, p58_g = P58_G, p58_h0 = 0.0f;
static int   p58_mirror = 1;
static uint32_t p58_h(uint32_t x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }
static float p58_ang[P58_NL], p58_spd[P58_NL], p58_hue[P58_NL];

static uint32_t p58_rs;
static float p58_rf(void)
{
    p58_rs ^= p58_rs << 13; p58_rs ^= p58_rs >> 17; p58_rs ^= p58_rs << 5;
    return (float)(p58_rs >> 8) * (1.0f / 16777216.0f);
}
static float p58_rn(float sd)
{
    float u1 = p58_rf(), u2 = p58_rf();
    if (u1 < 1e-6f) u1 = 1e-6f;
    return sd * sqrtf(-2.0f * logf(u1)) * cosf(P58_TAU * u2);
}

static void p58_init(void)
{
    int k;
    p58_rs = 0x58F0A17Bu;
    for (k = 0; k < P58_NL; k++) {
        p58_ang[k] = 1.57079632679f + 0.72f * sinf((float)k * 0.045f) + p58_rn(0.03f);
        p58_spd[k] = 2.45f + 0.30f * sinf((float)k * 0.11f) + p58_rn(0.08f);
        p58_hue[k] = fmodf((float)k * 0.011f, 1.0f);
    }
    p58_init_done = 1;
}

void pattern_058(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    /* tt cycles over 25 jet-parameter periods (3 frames per jet); 90000 frames
     * is also an exact whole number of hue turns, so the wrap is seamless. */
    float tt = (float)(frame % (P58_NL * 3 * 25)) + 240.0f;
    int kbase, kk, x, y;
    float hshift = tt * 0.0002f + p58_h0;
    (void)sl;
    if (!p58_init_done) p58_init();
    if (seed != p58_seed) {
        uint32_t a = p58_h(seed ^ 0x5858u), b = p58_h(a), c = p58_h(b), d = p58_h(c);
        p58_mirror = (a & 3u) != 0;                               /* 3 in 4 mirrored, 1 in 4 single */
        p58_cx     = P58_CX + ((float)((a >> 8) % 1000u) / 1000.0f - 0.5f)
                              * (p58_mirror ? 60.0f : 150.0f);    /* emitter off-centre */
        p58_spread = 0.55f + (float)(b % 1000u) * 0.0008f;        /* 0.55..1.35: tight jet .. wide fan */
        p58_pow    = 0.80f + (float)(c % 1000u) * 0.00045f;       /* 0.80..1.25: low arcs .. high */
        p58_g      = P58_G * (0.75f + (float)(d % 1000u) * 0.0005f);
        p58_h0     = (float)((d >> 12) % 1000u) * 0.001f;         /* where on the ramp it starts */
        p58_seed   = seed;
    }

    for (y = 0; y < P58_LH; y++) {
        float yy = (float)y / (float)(P58_LH - 1);
        float gg = 0.02f * yy, bb = 0.05f + 0.07f * yy;
        float *p = p58_acc + y * P58_LW * 3;
        for (x = 0; x < P58_LW; x++) {
            p[x * 3 + 0] = 0.0f; p[x * 3 + 1] = gg; p[x * 3 + 2] = bb;
        }
    }

    kbase = (int)(tt / 3.0f);            /* newest live jet index */
    for (kk = 0; kk < P58_LIVE; kk++) {
        int k = kbase - kk;
        int ki, ns, i;
        float age, vy, vx, aland, amax, oldf, cr, cg, cb, step;
        float lx = 0.0f, ly = 0.0f;
        ki = k % P58_NL; if (ki < 0) ki += P58_NL;
        age = tt - (float)k * 3.0f;
        if (age <= 0.0f) continue;
        {   float ang = 1.57079632679f + (p58_ang[ki] - 1.57079632679f) * p58_spread;
            vy = p58_spd[ki] * p58_pow * sinf(ang);
            vx = p58_spd[ki] * p58_pow * cosf(ang) * 0.62f; }
        aland = 2.0f * vy / p58_g + 8.0f;
        amax = age < aland ? age : aland;
        if (amax <= 0.0f) continue;
        ns = (int)(amax / 2.0f) + 2;
        oldf = expf(-(age > aland ? age - aland : 0.0f) * (1.0f / 260.0f));
        if (oldf < 0.01f) continue;
        p58_color(pal, p58_hue[ki] + hshift, 0.85f, 1.0f, &cr, &cg, &cb);
        step = amax / (float)(ns - 1);
        for (i = 0; i < ns; i++) {
            float a = step * (float)i;
            float u = a / (amax > 1.0f ? amax : 1.0f);
            float wgt = (0.20f + 0.80f * powf(u, 1.3f)) * oldf * 0.6f;
            lx = p58_cx + vx * a;
            ly = P58_BY - vy * a + 0.5f * p58_g * a * a;
            p58_splat(lx, ly, cr, cg, cb, wgt);
            if (p58_mirror) p58_splat(2.0f * p58_cx - lx, ly, cr, cg, cb, wgt);
        }
        if (age < aland) {               /* bright droplet heads */
            p58_splat(lx, ly, 0.9f, 0.9f, 0.9f, 0.9f);
            if (p58_mirror) p58_splat(2.0f * p58_cx - lx, ly, 0.9f, 0.9f, 0.9f, 0.9f);
        }
    }

    /* emitter glow, warm and pulsing */
    {
        float gr = 5.0f + 1.5f * sinf(tt * 0.05f);
        float cr, cg, cb;
        int i;
        p58_color(pal, 0.10f, 0.4f, 1.0f, &cr, &cg, &cb);
        for (i = 0; i < 26; i++) {
            float a = (float)i * (P58_TAU / 26.0f);
            p58_splat(p58_cx + gr * cosf(a), P58_BY + gr * sinf(a) * 0.5f,
                      cr, cg, cb, 0.7f);
        }
    }
    p58_blit(fb, w, h);
}
