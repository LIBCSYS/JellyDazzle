/* 159 Caustic Veil — the light net on the floor of a shallow pool.
 * A real (small) physical simulation: the surface is a sum of four travelling
 * sine waves h(x,y,t) = SUM A_i sin(k_i . x - w_i t); Snell's law at small
 * angles deflects a downward ray by an amount proportional to the surface
 * gradient, so every point of a photon grid is displaced by -d * grad h and
 * deposited on the floor. Where the surface is locally convex the photons pile
 * up and the classic bright caustic filaments appear — folds and cusps that
 * braid, pinch and re-open as the waves cross. The three channels are refracted
 * with slightly different constants, so the fold edges break into prismatic
 * fringes exactly as real water does. Phases are advanced incrementally along
 * each row, so the whole simulation costs four table lookups per photon.
 * Dark water with a bright sparse net — a natural overlay. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p159_up;

#define CW 440
#define CH 330
#define NW 4

static float p159_acc[CW * CH * 3];
static float p159_tmp[CW * CH * 3];
static unsigned char p159_img[CW * CH * 3];
static unsigned char p159_tone[2048];
static int *p159_xm;
static int p159_xmw;
static float p159_sin[4096];
static float p159_jit[512];
static float p159_kx[NW], p159_ky[NW], p159_om[NW], p159_am[NW];
static float p159_col[3][3];
static float p159_bg[3];
static float p159_hue0, p159_huew, p159_defl, p159_disp;
static uint32_t p159_seedc;
static int p159_ready, p159_tabs;

static uint32_t p159_rs;
static float p159_rf(void)
{
    p159_rs ^= p159_rs << 13; p159_rs ^= p159_rs >> 17; p159_rs ^= p159_rs << 5;
    return (float)(p159_rs >> 8) * (1.0f / 16777216.0f);
}

static void p159_pal3(const uint32_t *pal, float hue, float sat, float *o)
{
    uint32_t p; float r, g, b, mx;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
    mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    o[0] = (1.0f - sat) + sat * r / mx;
    o[1] = (1.0f - sat) + sat * g / mx;
    o[2] = (1.0f - sat) + sat * b / mx;
}

static void p159_setup(uint32_t seed)
{
    int i;
    p159_rs = seed ? seed ^ 0xC0FFA5u : 0xC0FFA5u;
    p159_rf(); p159_rf();
    for (i = 0; i < NW; i++) {
        float a = p159_rf() * 6.2831853f;
        float lam = 34.0f + p159_rf() * 78.0f;          /* canvas px         */
        float k = 6.2831853f / lam;
        p159_kx[i] = k * cosf(a); p159_ky[i] = k * sinf(a);
        p159_om[i] = (0.55f + p159_rf() * 0.85f) * sqrtf(k) * 0.040f;
        if (p159_rf() < 0.5f) p159_om[i] = -p159_om[i];
        p159_am[i] = (0.55f + p159_rf() * 0.75f) / k;
    }
    p159_hue0 = p159_rf();
    p159_huew = 0.04f + p159_rf() * 0.30f;
    p159_defl = 0.60f + p159_rf() * 0.55f;
    p159_disp = 0.05f + p159_rf() * 0.09f;
    if (!p159_tabs) {
        for (i = 0; i < 4096; i++)
            p159_sin[i] = sinf((float)i * (6.2831853f / 4096.0f));
        for (i = 0; i < 512; i++) p159_jit[i] = p159_rf() - 0.5f;
        for (i = 0; i < 2048; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (3.2f / 2048.0f)));
            p159_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p159_tabs = 1;
    }
    p159_ready = 1;
    p159_seedc = seed;
}

static void p159_splat1(float x, float y, int c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= CW - 1 || yi >= CH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p159_acc + (yi * CW + xi) * 3 + c;
    p[0] += (1.0f - fx) * (1.0f - fy) * w;
    p[3] += fx * (1.0f - fy) * w;
    p += CW * 3;
    p[0] += (1.0f - fx) * fy * w;
    p[3] += fx * fy * w;
}

static void p159_blur(void)
{
    int y, x, c;
    for (y = 1; y < CH - 1; y++)
        for (x = 1; x < CW - 1; x++) {
            int o = (y * CW + x) * 3;
            for (c = 0; c < 3; c++)
                p159_tmp[o + c] = p159_acc[o + c] * 0.36f
                    + 0.16f * (p159_acc[o + c - 3] + p159_acc[o + c + 3]
                             + p159_acc[o + c - CW * 3] + p159_acc[o + c + CW * 3]);
        }
    for (y = 1; y < CH - 1; y++)
        memcpy(p159_acc + (y * CW + 1) * 3, p159_tmp + (y * CW + 1) * 3,
               sizeof(float) * 3 * (CW - 2));
}

static void p159_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < CW * CH * 3; i++) {
        int ti = (int)(p159_acc[i] * 512.0f);
        p159_img[i] = p159_tone[ti < 0 ? 0 : ti > 2047 ? 2047 : ti];
    }
    if (p159_xmw != w) {
        free(p159_xm);
        p159_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p159_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p159_xmw = w;
    }
    jd_up_blit(&p159_up, fb, w, h, p159_img, CW, CH);
}

void pattern_159(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float ph0[NW], dpx[NW], dpy[NW], gx0[NW], gy0[NW];
    float defl, dsp, lw[3];
    int x, y, i, c;
    (void)sl;
    if (!p159_ready || p159_seedc != seed) p159_setup(seed);

    p159_pal3(pal, p159_hue0, 0.85f, p159_col[1]);
    p159_pal3(pal, p159_hue0 - p159_huew, 0.85f, p159_col[0]);
    p159_pal3(pal, p159_hue0 + p159_huew, 0.85f, p159_col[2]);
    p159_pal3(pal, p159_hue0 + 0.5f, 0.90f, p159_bg);

    for (i = 0; i < NW; i++) {
        ph0[i] = -p159_om[i] * t;
        dpx[i] = p159_kx[i]; dpy[i] = p159_ky[i];
        gx0[i] = p159_am[i] * p159_kx[i];
        gy0[i] = p159_am[i] * p159_ky[i];
    }
    for (i = 0; i < 3; i++) lw[i] = 0.052f + 0.115f * p159_col[1][i];
    defl = p159_defl * 26.0f * (1.0f + 0.10f * sinf(t * 0.00055f));
    dsp = p159_disp;

    /* dark water floor */
    for (y = 0; y < CH; y++) {
        float fy = (float)y / (float)CH;
        float k = 0.055f + 0.045f * (1.0f - fy);
        float *row = p159_acc + y * CW * 3;
        for (x = 0; x < CW; x++) {
            row[x * 3 + 0] = k * p159_bg[0] * 0.55f;
            row[x * 3 + 1] = k * p159_bg[1] * 0.75f;
            row[x * 3 + 2] = k * p159_bg[2];
        }
    }

    for (y = 0; y < CH; y++) {
        float ph[NW];
        float fy = (float)y;
        for (i = 0; i < NW; i++) ph[i] = ph0[i] + dpy[i] * fy;
        for (x = 0; x < CW; x++) {
            float gx = 0.0f, gy = 0.0f;
            for (i = 0; i < NW; i++) {
                int idx = (int)((ph[i] + dpx[i] * (float)x + 100.0f)
                                * (4096.0f / 6.2831853f)) & 4095;
                float cq = p159_sin[(idx + 1024) & 4095];
                gx += gx0[i] * cq; gy += gy0[i] * cq;
            }
            {
                float jx = p159_jit[(x * 7 + y * 29) & 511];
                float jy = p159_jit[(x * 23 + y * 11 + 137) & 511];
                float bx = (float)x + jx, by = (float)y + jy;
                for (c = 0; c < 3; c++) {
                    float s = defl * (1.0f + dsp * ((float)c - 1.0f));
                    p159_splat1(bx - gx * s, by - gy * s, c, lw[c]);
                }
            }
        }
    }
    p159_blur();
    p159_blit(fb, w, h);
}
