/* 193 Cord Braid — three plaited bands with real over-and-under.
 * Each cord rides a cylinder: y = A sin(phase), z = A cos(phase), phases evenly
 * spaced around the band, so the strands cross exactly the way a maypole plait
 * does. The crossings are not faked with draw order — every cord sample is
 * stamped through a z-buffer, so whichever cord is genuinely nearer occludes
 * the other, and each one also lays a slightly wider, darker casing just behind
 * itself, which gives the dark seam that makes Celtic knotwork readable. The
 * bands counter-scroll at slightly different rates over a long sine warp, so
 * the weave breathes and never lines up twice. Black between the bands. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p193_up;

#define P193_W 480
#define P193_H 360
#define P193_TAU 6.28318530717958647692f

static float p193_acc[P193_W * P193_H * 3];
static unsigned char p193_img[P193_W * P193_H * 3];
static unsigned char p193_tone[1024];
static int *p193_xm;
static int p193_xmw;
static int p193_tone_ok;
static uint32_t p193_rs = 1u;

static float p193_rf(void)
{
    p193_rs ^= p193_rs << 13; p193_rs ^= p193_rs >> 17; p193_rs ^= p193_rs << 5;
    return (float)(p193_rs >> 8) * (1.0f / 16777216.0f);
}

static void p193_tone_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (6.00f / 1024.0f)));
        p193_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
    }
    p193_tone_ok = 1;
}

/* palette sample, brightness-normalised so dark ramp zones still read as light */
static void p193_col(const uint32_t *pal, float hue, float lift, float *out)
{
    uint32_t p; float r, g, b, mx;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
    mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    out[0] = lift + (1.0f - lift) * r / mx;
    out[1] = lift + (1.0f - lift) * g / mx;
    out[2] = lift + (1.0f - lift) * b / mx;
}


static float p193_tmp[P193_W * P193_H * 3];

/* 5-tap soft glow, in place. Keeps line art from aliasing when it is scaled
 * up to 1280x960 and keeps frame-to-frame motion visually continuous. */
static void p193_blur(void)
{
    int y, x, c;
    for (y = 1; y < P193_H - 1; y++)
        for (x = 1; x < P193_W - 1; x++) {
            int o = (y * P193_W + x) * 3;
            for (c = 0; c < 3; c++)
                p193_tmp[o + c] = p193_acc[o + c] * 0.52f
                    + 0.12f * (p193_acc[o + c - 3] + p193_acc[o + c + 3]
                             + p193_acc[o + c - P193_W * 3] + p193_acc[o + c + P193_W * 3]);
        }
    for (y = 1; y < P193_H - 1; y++)
        memcpy(p193_acc + (y * P193_W + 1) * 3, p193_tmp + (y * P193_W + 1) * 3,
               sizeof(float) * 3 * (P193_W - 2));
}

static void p193_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P193_W * P193_H * 3; i++) {
        int ti = (int)(p193_acc[i] * 256.0f);
        p193_img[i] = p193_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p193_xmw != w) {
        free(p193_xm);
        p193_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p193_xm[x] = (int)(((long long)x * (P193_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p193_xmw = w;
    }
    jd_up_blit(&p193_up, fb, w, h, p193_img, P193_W, P193_H);
}

#define P193_NB 3
#define P193_NS 4

static uint32_t p193_seedc = 0xFFFFFFFFu;
static float p193_h0, p193_hw, p193_lam, p193_amp;
static float p193_spd[P193_NB], p193_yb[P193_NB];
static float p193_hue[P193_NB][P193_NS][3];
static float p193_z[P193_W * P193_H];

static void p193_build(uint32_t seed)
{
    int b;
    p193_rs = seed ? seed * 2654435761u + 0x7FEB352Du : 0x193u;
    p193_rf(); p193_rf();
    p193_h0  = p193_rf();
    p193_hw  = 0.05f + p193_rf() * 0.52f;
    p193_lam = 118.0f + p193_rf() * 70.0f;
    p193_amp = 30.0f + p193_rf() * 12.0f;
    for (b = 0; b < P193_NB; b++) {
        p193_spd[b] = ((b & 1) ? -1.0f : 1.0f) * (0.0026f + p193_rf() * 0.0022f);
        p193_yb[b] = (float)P193_H * (0.185f + 0.315f * (float)b);
    }
    p193_seedc = seed;
    if (!p193_tone_ok) p193_tone_init();
}

/* depth-tested disc: nearer z wins, so the plait occludes itself correctly */
static void p193_disc(float cx, float cy, float z, float r, const float *col, float k)
{
    int x0 = (int)(cx - r), x1 = (int)(cx + r) + 1;
    int y0 = (int)(cy - r), y1 = (int)(cy + r) + 1;
    int x, y;
    float r2 = r * r;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > P193_W) x1 = P193_W; if (y1 > P193_H) y1 = P193_H;
    for (y = y0; y < y1; y++) {
        float dy = (float)y + 0.5f - cy;
        for (x = x0; x < x1; x++) {
            float dx = (float)x + 0.5f - cx;
            float d2 = dx * dx + dy * dy, s;
            int o;
            if (d2 > r2) continue;
            o = y * P193_W + x;
            if (z <= p193_z[o]) continue;
            p193_z[o] = z;
            s = k * (0.42f + 0.58f * (1.0f - d2 / r2));
            p193_acc[o * 3 + 0] = col[0] * s;
            p193_acc[o * 3 + 1] = col[1] * s;
            p193_acc[o * 3 + 2] = col[2] * s;
        }
    }
}

void pattern_193(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, kw, amp;
    int b, s, i;
    (void)sl;
    if (p193_seedc != seed) p193_build(seed);
    for (b = 0; b < P193_NB; b++)
        for (s = 0; s < P193_NS; s++)
            p193_col(pal, p193_h0 + p193_hw * (0.26f * (float)b
                     + 0.62f * (float)s / (float)P193_NS), 0.16f, p193_hue[b][s]);
    memset(p193_acc, 0, sizeof p193_acc);
    for (i = 0; i < P193_W * P193_H; i++) p193_z[i] = -1e9f;

    kw = P193_TAU / p193_lam;
    amp = p193_amp * (1.0f + 0.10f * sinf(t * 0.00047f));
    for (b = 0; b < P193_NB; b++) {
        float drift = t * p193_spd[b];
        float ybase = p193_yb[b] + 7.0f * sinf(t * 0.00061f + (float)b * 2.1f);
        for (s = 0; s < P193_NS; s++) {
            const float *col = p193_hue[b][s];
            float dark[3];
            float x;
            dark[0] = col[0] * 0.06f; dark[1] = col[1] * 0.06f; dark[2] = col[2] * 0.06f;
            float dir = (s & 1) ? -1.0f : 1.0f;
            for (x = -6.0f; x < (float)P193_W + 6.0f; x += 0.72f) {
                float ph = (x * kw + drift) * dir + (float)s * (P193_TAU / (float)P193_NS);
                float warp = 9.0f * sinf(x * 0.0105f + t * 0.0016f + (float)b);
                float y = ybase + warp + amp * sinf(ph);
                float z = amp * cosf(ph);
                float sh = 0.52f + 0.48f * (z / amp);
                p193_disc(x, y, z - 2.6f, 5.7f, dark, 1.0f);
                p193_disc(x, y, z, 4.0f, col, sh);
            }
        }
    }
    p193_blur();
    p193_blit(fb, w, h);
}
