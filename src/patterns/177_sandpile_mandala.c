/* 177 Sandpile Mandala — the abelian sandpile, relaxing on camera.
 * A disc of cells is loaded with far more grains than it can hold and then left
 * to topple: any cell with four or more grains gives one to each neighbour, and
 * because the rule is abelian the order does not matter, so the pile can be
 * relaxed a few sweeps at a time and still converge to the one true answer.
 * What comes out is not a blur but the famous fractal medallion — flat plateaus
 * of constant height, straight facet boundaries, triangular and square patches
 * self-similar at every scale — spreading outward frame by frame from the
 * middle of a black field. Only four colours are ever used, one per grain
 * count, which makes this the starkest routine in the set. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p177_up;

#define P177_W 480
#define P177_H 360
#define P177_G 501
#define P177_SW 6

static float p177_acc[P177_W * P177_H * 3];
static unsigned char p177_img[P177_W * P177_H * 3];
static unsigned char p177_tone[1024];
static uint16_t p177_h[P177_G * P177_G];
static unsigned char p177_seen[P177_G * P177_G];
static float p177_disp[P177_G * P177_G];
static int *p177_xm;
static int p177_xmw;
static int p177_ready;
static uint32_t p177_seedc;
static int p177_armed;
static int p177_x0, p177_x1, p177_y0, p177_y1;
static int p177_sx[P177_H];
static float p177_sf[P177_H];
static int p177_smap;
static float p177_lev[4][3];
static float p177_hue0, p177_huew, p177_load;

static uint32_t p177_rs;
static float p177_rf(void)
{
    p177_rs ^= p177_rs << 13; p177_rs ^= p177_rs >> 17; p177_rs ^= p177_rs << 5;
    return (float)(p177_rs >> 8) * (1.0f / 16777216.0f);
}

static void p177_setup(uint32_t seed)
{
    int x, y, r0;
    float load;
    p177_rs = seed ? seed ^ 0x5A4D1177u : 0x5A4D1177u;
    p177_rf(); p177_rf();
    p177_hue0 = p177_rf();
    p177_huew = 0.16f + p177_rf() * 0.60f;
    p177_load = 7.0f + p177_rf() * 5.0f;
    load = p177_load;
    r0 = 64 + (int)(p177_rf() * 22.0f);
    memset(p177_h, 0, sizeof p177_h);
    memset(p177_seen, 0, sizeof p177_seen);
    memset(p177_disp, 0, sizeof p177_disp);
    for (y = -r0; y <= r0; y++)
        for (x = -r0; x <= r0; x++)
            if (x * x + y * y <= r0 * r0) {
                int i = (P177_G / 2 + y) * P177_G + (P177_G / 2 + x);
                p177_h[i] = (uint16_t)load;
                p177_seen[i] = 1;
            }
    p177_x0 = P177_G / 2 - r0 - 1; p177_x1 = P177_G / 2 + r0 + 1;
    p177_y0 = P177_G / 2 - r0 - 1; p177_y1 = P177_G / 2 + r0 + 1;
    if (!p177_ready) {
        int i;
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (3.2f / 1024.0f)));
            p177_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p177_ready = 1;
    }
    p177_seedc = seed;
    p177_armed = 1;
}

static void p177_levels(const uint32_t *pal)
{
    static const float bright[4] = {0.18f, 0.44f, 0.70f, 0.94f};
    int i;
    for (i = 0; i < 4; i++) {
        float hue = p177_hue0 + p177_huew * ((float)i / 3.0f);
        float r, g, b, mx;
        uint32_t p;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p177_lev[i][0] = (0.05f + 0.95f * r / mx) * bright[i];
        p177_lev[i][1] = (0.05f + 0.95f * g / mx) * bright[i];
        p177_lev[i][2] = (0.05f + 0.95f * b / mx) * bright[i];
    }
}

static void p177_relax(void)
{
    int pass, x, y;
    int nx0, nx1, ny0, ny1;
    for (pass = 0; pass < P177_SW; pass++) {
        nx0 = P177_G; nx1 = -1; ny0 = P177_G; ny1 = -1;
        for (y = p177_y0; y <= p177_y1; y++) {
            uint16_t *row = p177_h + y * P177_G;
            for (x = p177_x0; x <= p177_x1; x++) {
                unsigned v = row[x];
                unsigned n;
                if (v < 4u) continue;
                n = v >> 2;
                row[x] = (uint16_t)(v - (n << 2));
                row[x - 1] += (uint16_t)n; row[x + 1] += (uint16_t)n;
                row[x - P177_G] += (uint16_t)n; row[x + P177_G] += (uint16_t)n;
                p177_seen[y * P177_G + x - 1] = 1;
                p177_seen[y * P177_G + x + 1] = 1;
                p177_seen[(y - 1) * P177_G + x] = 1;
                p177_seen[(y + 1) * P177_G + x] = 1;
                if (x < nx0) nx0 = x;
                if (x > nx1) nx1 = x;
                if (y < ny0) ny0 = y;
                if (y > ny1) ny1 = y;
            }
        }
        if (nx1 < nx0) break;                     /* fully relaxed           */
        p177_x0 = nx0 > 2 ? nx0 - 2 : 2;
        p177_x1 = nx1 < P177_G - 3 ? nx1 + 2 : P177_G - 3;
        p177_y0 = ny0 > 2 ? ny0 - 2 : 2;
        p177_y1 = ny1 < P177_G - 3 ? ny1 + 2 : P177_G - 3;
    }
}

static void p177_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P177_W * P177_H * 3; i++) {
        int ti = (int)(p177_acc[i] * 256.0f);
        p177_img[i] = p177_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p177_xmw != w) {
        free(p177_xm);
        p177_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p177_xm[x] = (int)(((long long)x * (P177_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p177_xmw = w;
    }
    jd_up_blit(&p177_up, fb, w, h, p177_img, P177_W, P177_H);
}

void pattern_177(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int x, y, i;
    float sgrid, glow;
    if (!p177_armed || p177_seedc != seed || sl == 0) p177_setup(seed);
    p177_levels(pal);
    p177_relax();

    /* temporal damping: cells in flight flicker between counts, and the eye
       must never see that — it sees a slow settle instead */
    {
        int dx0 = p177_x0 - 3 < 0 ? 0 : p177_x0 - 3;
        int dx1 = p177_x1 + 3 > P177_G - 1 ? P177_G - 1 : p177_x1 + 3;
        for (y = p177_y0 - 3; y <= p177_y1 + 3; y++) {
            int yy = y < 0 ? 0 : y > P177_G - 1 ? P177_G - 1 : y;
            const uint16_t *hr = p177_h + yy * P177_G;
            float *dr = p177_disp + yy * P177_G;
            for (x = dx0; x <= dx1; x++) {
                float tgt = (float)(hr[x] > 3u ? 3u : hr[x]);
                dr[x] += (tgt - dr[x]) * 0.10f;
            }
        }
    }
    memset(p177_acc, 0, sizeof p177_acc);
    sgrid = (float)(P177_G - 2) / (float)P177_H;
    if (!p177_smap) {
        for (x = 0; x < P177_H; x++) {
            float g = 0.5f + (float)x * sgrid;
            p177_sx[x] = (int)g;
            p177_sf[x] = g - (float)(int)g;
        }
        p177_smap = 1;
    }
    glow = 0.92f + 0.08f * sinf((float)frame * 0.0011f);

    for (y = 0; y < P177_H; y++) {
        float gy = 0.5f + (float)y * sgrid;
        int iy = (int)gy;
        float fy = gy - (float)iy;
        const float *d0 = p177_disp + iy * P177_G;
        const float *d1 = d0 + P177_G;
        const unsigned char *s0 = p177_seen + iy * P177_G;
        const unsigned char *s1 = s0 + P177_G;
        float *dst = p177_acc + (y * P177_W + (P177_W - P177_H) / 2) * 3;
        for (x = 0; x < P177_H; x++, dst += 3) {
            int ix = p177_sx[x];
            float fx = p177_sf[x];
            float a, b, v, m, lo, hi3;
            const float *ca, *cb;
            int k;
            if (!(s0[ix] | s0[ix + 1] | s1[ix] | s1[ix + 1])) continue;
            a = d0[ix] + (d0[ix + 1] - d0[ix]) * fx;
            b = d1[ix] + (d1[ix + 1] - d1[ix]) * fx;
            v = a + (b - a) * fy;
            if (v < 0.0f) v = 0.0f;
            if (v > 2.999f) v = 2.999f;
            k = (int)v; m = v - (float)k;
            ca = p177_lev[k]; cb = p177_lev[k + 1 < 4 ? k + 1 : 3];
            lo = 1.0f - m; hi3 = m;
            dst[0] = (ca[0] * lo + cb[0] * hi3) * glow;
            dst[1] = (ca[1] * lo + cb[1] * hi3) * glow;
            dst[2] = (ca[2] * lo + cb[2] * hi3) * glow;
        }
    }
    (void)i;
    p177_blit(fb, w, h);
}
