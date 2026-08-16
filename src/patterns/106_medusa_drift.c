/* 106 Medusa Drift — a bloom of jellyfish, lit from inside.
 * Seven medusae rise through dark water. Each one is a jet: the bell contracts
 * over about a second (narrow and tall), relaxes over two (wide and flat), and
 * the animal only accelerates while it is squeezing — the drift is integrated
 * from that thrust, so the rise is the pulse, not a separate animation. The
 * bell is an ellipse field, bright on the rim and translucent inside; eight
 * tentacles hang from each margin as sine filaments whose phase lags the bell
 * by a quarter period, so they crack and stream a beat behind every squeeze.
 * Hue is per animal from the live palette, with the oral arms a few steps off
 * the bell. Water is near-black and the animals are small, so it composites
 * over a ground instead of hiding it. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p106_up;

#define P106_LW 480
#define P106_LH 360
#define P106_NJ 7
#define P106_NT 9                      /* tentacles per animal */

static float p106_acc[P106_LW * P106_LH * 3];
static uint8_t p106_img[P106_LW * P106_LH * 3];
static int *p106_xm;
static int p106_xm_w;
static uint8_t p106_tone[2048];
static uint8_t p106_ramp[256][3];
static uint16_t p106_shell[1024];
static float p106_jy[P106_NJ], p106_jx[P106_NJ], p106_jph[P106_NJ];
static float p106_jsz[P106_NJ], p106_jsw[P106_NJ];
static uint8_t p106_jh[P106_NJ];
static int p106_ready;

static void p106_init(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 1.0f - expf(-(float)i * (4.6f / 2048.0f));
        p106_tone[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    for (i = 0; i < 1024; i++) {
        /* radial profile of the bell: bright rim at d = 1, translucent inside */
        float d = (float)i * (1.6f / 1024.0f);
        float rim = expf(-(d - 1.0f) * (d - 1.0f) * 52.0f);
        float halo = 0.22f * expf(-(d - 1.0f) * (d - 1.0f) * 3.4f);
        float body = d < 1.0f ? (0.13f * (1.0f - d * d * 0.5f)) : 0.0f;
        p106_shell[i] = (uint16_t)((rim + halo + body) * 2600.0f);
    }
    for (i = 0; i < P106_NJ; i++) {
        p106_jx[i] = (float)(i * 67 % 400) + 40.0f;
        p106_jy[i] = (float)(i * 53 % 300) + 30.0f;
        p106_jph[i] = (float)i * 0.897f;
        p106_jsz[i] = 17.0f + (float)(i % 4) * 5.0f;
        p106_jsw[i] = 0.0035f + 0.0011f * (float)(i % 5);
        p106_jh[i] = (uint8_t)(i * 37);
    }
    p106_ready = 1;
}

static void p106_build_ramp(const uint32_t *pal, int base)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * 128) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p106_ramp[i][0] = p106_ramp[i-1][0];
                     p106_ramp[i][1] = p106_ramp[i-1][1];
                     p106_ramp[i][2] = p106_ramp[i-1][2]; }
            else   { p106_ramp[i][0] = p106_ramp[i][1] = p106_ramp[i][2] = 210; }
            continue;
        }
        p106_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p106_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p106_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static void p106_dot(float x, float y, const uint8_t *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, *p;
    float r = c[0] * wgt, g = c[1] * wgt, b = c[2] * wgt;
    if ((unsigned)xi >= P106_LW - 1 || (unsigned)yi >= P106_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p106_acc + (yi * P106_LW + xi) * 3;
    {
        float w00 = (1.0f - fx) * (1.0f - fy), w10 = fx * (1.0f - fy);
        float w01 = (1.0f - fx) * fy, w11 = fx * fy;
        p[0] += r * w00; p[1] += g * w00; p[2] += b * w00;
        p[3] += r * w10; p[4] += g * w10; p[5] += b * w10;
        p += P106_LW * 3;
        p[0] += r * w01; p[1] += g * w01; p[2] += b * w01;
        p[3] += r * w11; p[4] += g * w11; p[5] += b * w11;
    }
}

/* the bell: an ellipse field sampled over its bounding box */
static void p106_bell(float cx, float cy, float a, float b,
                      const uint8_t *col, float amp)
{
    int x0 = (int)(cx - a * 1.3f), x1 = (int)(cx + a * 1.3f) + 1;
    int y0 = (int)(cy - b * 1.3f), y1 = (int)(cy + b * 1.3f) + 1;
    float ia = 1.0f / (a * a), ib = 1.0f / (b * b);
    int x, y;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > P106_LW) x1 = P106_LW;
    if (y1 > P106_LH) y1 = P106_LH;
    for (y = y0; y < y1; y++) {
        float dy = (float)y + 0.5f - cy;
        float qy = dy * dy * ib;
        float *p = p106_acc + (y * P106_LW + x0) * 3;
        /* the bell is open below: fade the lower half out */
        float open = dy > 0.0f ? 1.0f - dy / (b * 1.35f) : 1.0f;
        if (open < 0.0f) open = 0.0f;
        for (x = x0; x < x1; x++, p += 3) {
            float dx = (float)x + 0.5f - cx;
            float q = dx * dx * ia + qy;
            int si = (int)(q * 640.0f);
            float v;
            if (si > 1023) continue;
            v = (float)p106_shell[si] * (1.0f / 2048.0f) * amp * open;
            p[0] += col[0] * v; p[1] += col[1] * v; p[2] += col[2] * v;
        }
    }
}

static void p106_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p106_xm_w != w) {
        free(p106_xm);
        p106_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p106_xm[x] = (int)(((long long)x * (P106_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p106_xm_w = w;
    }
    jd_up_blit(&p106_up, fb, w, h, p106_img, P106_LW, P106_LH);
}

void pattern_106(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    int i, j, k, n3 = P106_LW * P106_LH * 3, hbase;
    (void)sl;

    if (!p106_ready) p106_init();
    hbase = (int)(t * 1.1f) + (int)(seed & 32767);
    p106_build_ramp(pal, hbase);
    memset(p106_acc, 0, sizeof p106_acc);

    for (i = 0; i < P106_NJ; i++) {
        float ph = p106_jph[i] + t * p106_jsw[i] + (float)(seed & 255) * 0.004f;
        /* squeeze: fast contraction, slow relaxation (a skewed cosine) */
        float sq = 0.5f - 0.5f * cosf(ph + 0.55f * sinf(ph));
        float a = p106_jsz[i] * (1.30f - 0.40f * sq);
        float b = p106_jsz[i] * (0.62f + 0.42f * sq);
        float cx, cy, amp;
        const uint8_t *bell = p106_ramp[(hbase / 16 + p106_jh[i]) & 255];
        const uint8_t *arms = p106_ramp[(hbase / 16 + p106_jh[i] + 26) & 255];
        /* rise: the animal only gains height while it squeezes */
        p106_jy[i] -= (0.30f + 0.85f * sq * sq) * 0.42f;
        if (p106_jy[i] < -40.0f) p106_jy[i] += (float)P106_LH + 80.0f;
        p106_jx[i] += 0.16f * sinf(t * 0.0021f + (float)i * 1.7f);
        cx = p106_jx[i]; cy = p106_jy[i];
        amp = 0.85f;
        p106_bell(cx, cy, a, b, bell, amp);
        for (j = 0; j < 5; j++) {          /* radial canals inside the bell */
            float u = ((float)j - 2.0f) * 0.34f;
            int ns = (int)(b * 1.5f);
            for (k = 0; k < ns; k++) {
                float s = (float)k / (float)ns;
                float px = cx + u * a * (0.35f + 0.65f * s);
                float py = cy - b * 0.55f + s * b * 1.25f;
                p106_dot(px, py, bell, 0.045f * (0.4f + 0.6f * s));
            }
        }
        for (j = 0; j < P106_NT; j++) {
            float u = ((float)j + 0.5f) / (float)P106_NT;
            float bx = cx + (u * 2.0f - 1.0f) * a * 0.94f;
            float sgn = (u * 2.0f - 1.0f);
            float by = cy + b * 0.72f * (1.0f - 0.55f * sgn * sgn);
            float lag = ph - 1.25f;
            float len = p106_jsz[i] * (3.4f + 1.5f * (1.0f - sq));
            int ns = (int)(len * 0.9f);
            for (k = 0; k < ns; k++) {
                float s = (float)k / (float)ns;
                float wig = sinf(lag + s * 5.4f + (float)j * 0.9f)
                          * (3.2f + 11.0f * s) * (0.35f + 0.65f * sq);
                float px = bx + sgn * s * a * 0.55f + wig * 0.55f;
                float py = by + s * len;
                float fade = (1.0f - s) * (1.0f - s * 0.4f);
                p106_dot(px, py, arms, 0.20f * fade);
            }
        }
    }

    for (i = 0; i < n3; i++) {
        int ti = (int)(p106_acc[i] * 2.6f);
        if (ti > 2047) ti = 2047;
        p106_img[i] = p106_tone[ti];
    }
    p106_blit(fb, w, h);
}
