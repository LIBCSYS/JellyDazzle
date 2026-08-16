/* 111 Physarum Veins — slime mould, simulated honestly. 42000 agents crawl over
 * a chemo-attractant field; each frame every agent samples the field 9 px ahead
 * and 40 degrees to either side, steers toward whichever sample is strongest,
 * steps forward and deposits its own colour behind it. The field is diffused
 * (1-2-1 separable) and decayed each frame. Nobody draws a vein: the veins are
 * what a positive feedback loop between deposition and steering builds, and
 * they keep re-routing forever — trunk routes thicken, starved branches fade,
 * junctions migrate. The steering angle and sensor distance breathe on slow
 * sines so the network alternates between a fine mesh and a few fat highways.
 * Glowing filaments on black: an overlay layer.
 */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
static jd_up p111_up;

#define P111_LW 640
#define P111_LH 480
#define P111_N  (P111_LW * P111_LH)
#define P111_AG 42000
#define P111_TAU 6.28318530717959f

static float p111_tr[P111_N * 3];
static float p111_tmp[P111_N * 3];
static unsigned char p111_img[P111_N * 3];
static float p111_ax[P111_AG], p111_ay[P111_AG], p111_ah[P111_AG];
static unsigned char p111_hue[P111_AG];
static int p111_init;
static float p111_ramp[256][3];
static uint32_t p111_rs = 0x9E3779B9u;

static uint32_t p111_rnd(void)
{
    p111_rs ^= p111_rs << 13; p111_rs ^= p111_rs >> 17; p111_rs ^= p111_rs << 5;
    return p111_rs;
}

static void p111_ramp_build(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i * 128) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 8.0f) mx = 8.0f;
        p111_ramp[i][0] = r / mx; p111_ramp[i][1] = g / mx; p111_ramp[i][2] = b / mx;
    }
}

/* field strength at a point, nearest sample, toroidal */
static float p111_sense(float x, float y)
{
    int xi = (int)x, yi = (int)y;
    const float *p;
    if (xi < 0) xi += P111_LW; else if (xi >= P111_LW) xi -= P111_LW;
    if (yi < 0) yi += P111_LH; else if (yi >= P111_LH) yi -= P111_LH;
    p = p111_tr + ((size_t)yi * P111_LW + (size_t)xi) * 3;
    return p[0] + p[1] + p[2];
}

static void p111_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1, w2, w3; float *p;
    if (xi < 0 || yi < 0 || xi >= P111_LW - 1 || yi >= P111_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    w2 = (1.0f - fx) * fy * w;         w3 = fx * fy * w;
    p = p111_tr + ((size_t)yi * P111_LW + xi) * 3;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P111_LW * 3;
    p[0] += c[0] * w2; p[1] += c[1] * w2; p[2] += c[2] * w2;
    p[3] += c[0] * w3; p[4] += c[1] * w3; p[5] += c[2] * w3;
}

static void p111_diffuse(float decay)
{
    int x, y, c;
    for (y = 0; y < P111_LH; y++) {
        const float *s = p111_tr + (size_t)y * P111_LW * 3;
        float *d = p111_tmp + (size_t)y * P111_LW * 3;
        for (x = 0; x < P111_LW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < P111_LW - 1 ? x + 1 : P111_LW - 1;
            for (c = 0; c < 3; c++)
                d[x * 3 + c] = 0.25f * (s[xm * 3 + c] + s[xp * 3 + c]) +
                               0.50f * s[x * 3 + c];
        }
    }
    for (x = 0; x < P111_LW; x++)
        for (y = 0; y < P111_LH; y++) {
            int ym = y > 0 ? y - 1 : 0, yp = y < P111_LH - 1 ? y + 1 : P111_LH - 1;
            for (c = 0; c < 3; c++) {
                size_t o = (size_t)x * 3 + (size_t)c;
                p111_tr[(size_t)y * P111_LW * 3 + o] = decay *
                    (0.25f * (p111_tmp[(size_t)ym * P111_LW * 3 + o] +
                              p111_tmp[(size_t)yp * P111_LW * 3 + o]) +
                     0.50f * p111_tmp[(size_t)y * P111_LW * 3 + o]);
            }
        }
}

static void p111_blit(uint32_t *fb, int w, int h)
{
    int i, n = P111_N * 3;
    for (i = 0; i < n; i++) {
        float cc = p111_tr[i], v = 255.0f * cc / (0.55f + cc);
        p111_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    jd_up_blit(&p111_up, fb, w, h, p111_img, P111_LW, P111_LH);
}

void pattern_111(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sp = (float)(seed & 1023) * 0.006136f;
    float sd, sa, turn, step, dep;
    int i, hbase;
    (void)sl;

    p111_ramp_build(pal);
    if (!p111_init) {
        memset(p111_tr, 0, sizeof p111_tr);
        for (i = 0; i < P111_AG; i++) {
            p111_ax[i] = (float)(p111_rnd() % (P111_LW * 64u)) * (1.0f / 64.0f);
            p111_ay[i] = (float)(p111_rnd() % (P111_LH * 64u)) * (1.0f / 64.0f);
            p111_ah[i] = (float)(p111_rnd() % 65536u) * (P111_TAU / 65536.0f);
            p111_hue[i] = (unsigned char)(p111_rnd() >> 24);
        }
        p111_init = 1;
    }

    sd   = 8.5f + 2.2f * sinf(0.00061f * t + sp);         /* sensor distance */
    sa   = 0.40f + 0.12f * sinf(0.00043f * t + 1.3f);     /* sensor angle    */
    turn = 0.72f + 0.16f * sinf(0.00037f * t + 2.7f);     /* turn rate       */
    step = 0.95f;
    dep  = 0.155f;
    hbase = (int)(t * 0.035f + sp * 30.0f);

    p111_diffuse(0.930f);

    for (i = 0; i < P111_AG; i++) {
        float hd = p111_ah[i], x = p111_ax[i], y = p111_ay[i];
        float cf = cosf(hd), sf = sinf(hd);
        float cl = cosf(hd - sa), sl2 = sinf(hd - sa);
        float cr = cosf(hd + sa), sr = sinf(hd + sa);
        float f = p111_sense(x + cf * sd, y + sf * sd);
        float l = p111_sense(x + cl * sd, y + sl2 * sd);
        float r = p111_sense(x + cr * sd, y + sr * sd);
        float jit;
        if (l > f && l >= r) hd -= turn;
        else if (r > f && r > l) hd += turn;
        jit = (float)(int)(p111_rnd() >> 20) * (1.0f / 2048.0f) - 1.0f;
        hd += jit * 0.045f;
        x += cosf(hd) * step; y += sinf(hd) * step;
        if (x < 0.0f) x += (float)P111_LW; else if (x >= (float)P111_LW) x -= (float)P111_LW;
        if (y < 0.0f) y += (float)P111_LH; else if (y >= (float)P111_LH) y -= (float)P111_LH;
        p111_ax[i] = x; p111_ay[i] = y; p111_ah[i] = hd;
        {   /* hue follows the direction of travel, so a vein keeps one colour
             * along its length; position dominates so colour stays coherent */
            int hi = (hbase + (int)(x * 0.21f + y * 0.15f)
                      + (int)(hd * 9.0f)) & 255;
            p111_splat(x, y, p111_ramp[hi], dep);
        }
    }
    p111_blit(fb, w, h);
}
