/* 009 lissa_web — a detuned 3:4 Lissajous ribbon stamped N times around the
 * centre (N steps 5/7/9/12/8/6), splatted into a glow buffer over a dim
 * radial ground: neon guilloche string-art that re-laces itself. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P9_IW 320
#define P9_IH 240
#define P9_NS 4200                 /* curve samples in the trailing window */
#define P9_XF 120                  /* frames of lacing-to-lacing cross-fade */

static float p9_acc[P9_IW * P9_IH];
static float p9_blur[P9_IW * P9_IH];
static float p9_px[P9_NS], p9_py[P9_NS], p9_wt[P9_NS];
static uint16_t p9_idx[P9_IW * P9_IH];
static uint8_t p9_gl[P9_IW * P9_IH];
static uint16_t p9_ground[P9_IW * P9_IH];
static uint16_t p9_hrad[P9_IW * P9_IH];
static uint8_t p9_glut[1024];
static int p9_init;

#define P9_SCALE 8192.0f
#define P9_BMAX  12.0f

/* Bilinear upsample of the low-res (index, glow) pair straight to the target,
 * modulating palette brightness by the glow so the ground stays near-black. */
static void p9_blit(uint32_t *fb, int w, int h, const uint32_t *pal, uint32_t off) {
    uint32_t sx = (uint32_t)(((P9_IW - 1) << 16) - 1) / (uint32_t)(w > 1 ? w - 1 : 1);
    uint32_t sy = (uint32_t)(((P9_IH - 1) << 16) - 1) / (uint32_t)(h > 1 ? h - 1 : 1);
    for (int y = 0; y < h; y++) {
        uint32_t ay = sy * (uint32_t)y;
        int iy = (int)(ay >> 16), fy = (int)((ay >> 8) & 255);
        const uint16_t *i0 = p9_idx + iy * P9_IW, *i1 = i0 + P9_IW;
        const uint8_t *g0 = p9_gl + iy * P9_IW, *g1 = g0 + P9_IW;
        uint32_t *dst = fb + (long)y * w;
        uint32_t ax = 0;
        for (int x = 0; x < w; x++) {
            int ix = (int)(ax >> 16), fx = (int)((ax >> 8) & 255);
            int a = i0[ix], b = i0[ix + 1], c = i1[ix], d = i1[ix + 1];
            int top = a + (((b - a) * fx) >> 8);
            int bot = c + (((d - c) * fx) >> 8);
            int vi = top + (((bot - top) * fy) >> 8);
            int ga = g0[ix], gb = g0[ix + 1], gc = g1[ix], gd = g1[ix + 1];
            int gt = ga + (((gb - ga) * fx) >> 8);
            int gbo = gc + (((gd - gc) * fx) >> 8);
            int g = gt + (((gbo - gt) * fy) >> 8);
            uint32_t p = pal[((uint32_t)vi + off) & JD_PAL_MASK];
            int lum = 26 + ((229 * g) >> 8);        /* 0.10 ground .. 1.0 thread */
            uint32_t r = (((p >> 16) & 255) * (uint32_t)lum) >> 8;
            uint32_t gg = (((p >> 8) & 255) * (uint32_t)lum) >> 8;
            uint32_t bl = ((p & 255) * (uint32_t)lum) >> 8;
            dst[x] = 0xFF000000u | (r << 16) | (gg << 8) | bl;
            ax += sx;
        }
    }
}

void pattern_009(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl; (void)seed;
    static const int seq[6] = {5, 7, 9, 12, 8, 6};
    if (!p9_init) {
        p9_init = 1;
        for (int i = 0; i < P9_NS; i++)
            p9_wt[i] = 0.15f + 0.85f * ((float)i / (float)(P9_NS - 1));
        for (int i = 0; i < 1024; i++) {
            float xv = (float)i * (P9_BMAX / 1023.0f);
            float g = 1.0f - expf(-xv * 0.55f);
            p9_glut[i] = (uint8_t)(g * 255.0f + 0.5f);
        }
        for (int y = 0; y < P9_IH; y++)
            for (int x = 0; x < P9_IW; x++) {
                float X = (float)x - P9_IW * 0.5f, Y = (float)y - P9_IH * 0.5f;
                float rs = sqrtf(X * X + Y * Y);
                p9_ground[y * P9_IW + x] = (uint16_t)(rs * 0.001f * P9_SCALE + 4096.0f);
                p9_hrad[y * P9_IW + x] = (uint16_t)(rs * 0.0022f * P9_SCALE);
            }
    }
    float t = (float)frame;
    /* The stamp count used to snap to the next entry of seq[] the instant
     * frame/320 ticked, re-lacing the whole web in one frame (measured
     * delta ~29 every 320 frames — a slow strobe).  Cross-fade the two
     * lacings instead: over the last P9_XF frames of each leg both arm
     * counts are stamped with complementary smootherstep weights, so the
     * web dissolves from one lacing into the next.  Outside the window
     * the second arm's weight is zero and it is skipped, so the common
     * case costs exactly what it did before. */
    int leg = frame / 320, pos = frame % 320;
    int arm_n[2]; float arm_w[2]; float mb = 0.0f;
    if (pos >= 320 - P9_XF) {
        float u = (float)(pos - (320 - P9_XF)) * (1.0f / (float)P9_XF);
        mb = u * u * u * (u * (u * 6.0f - 15.0f) + 10.0f);
    }
    arm_n[0] = seq[leg % 6];       arm_w[0] = 1.0f - mb;
    arm_n[1] = seq[(leg + 1) % 6]; arm_w[1] = mb;
    /* temporal rates halved vs the lab prototype to satisfy the motion law */
    float sb = t * 0.006f, phx = t * 0.00075f, stamp = t * 0.0009f;
    for (int i = 0; i < P9_NS; i++) {
        float s = (float)i * 0.006f + sb;
        p9_px[i] = 92.0f * sinf(3.0f * s + phx);
        p9_py[i] = 92.0f * sinf(4.0048f * s);
    }
    for (int i = 0; i < P9_IW * P9_IH; i++) p9_acc[i] = 0.0f;
    float cx = P9_IW * 0.5f, cy = P9_IH * 0.5f;
    for (int a = 0; a < 2; a++) {
        int N = arm_n[a];
        float armw = arm_w[a];
        if (armw < 0.004f) continue;          /* the usual single-arm path */
        for (int k = 0; k < N; k++) {
            float ang = (float)k * (6.2831853f / (float)N) + stamp;
            float ca = cosf(ang), sa = sinf(ang);
            for (int i = 0; i < P9_NS; i++) {
                float px = p9_px[i], py = p9_py[i];
                float gx = px * ca - py * sa + cx;
                float gy = px * sa + py * ca + cy;
                int ix = (int)floorf(gx), iy = (int)floorf(gy);
                if (ix < 0 || ix >= P9_IW - 1 || iy < 0 || iy >= P9_IH - 1) continue;
                float fx = gx - (float)ix, fy = gy - (float)iy;
                float wq = p9_wt[i] * armw;
                float w0 = wq * (1.0f - fx), w1 = wq * fx;
                float *p = p9_acc + iy * P9_IW + ix;
                p[0] += w0 * (1.0f - fy);
                p[1] += w1 * (1.0f - fy);
                p[P9_IW] += w0 * fy;
                p[P9_IW + 1] += w1 * fy;
            }
        }
    }
    /* one 5-tap box soften (edges just reuse the centre tap) */
    for (int y = 0; y < P9_IH; y++) {
        const float *src = p9_acc + y * P9_IW;
        const float *up = (y > 0) ? src - P9_IW : src;
        const float *dn = (y < P9_IH - 1) ? src + P9_IW : src;
        float *dst = p9_blur + y * P9_IW;
        for (int x = 0; x < P9_IW; x++) {
            float c = src[x];
            float l = (x > 0) ? src[x - 1] : c;
            float r = (x < P9_IW - 1) ? src[x + 1] : c;
            dst[x] = c + l + r + up[x] + dn[x];
        }
    }
    const float gsc = 1023.0f / P9_BMAX;
    for (int i = 0; i < P9_IW * P9_IH; i++) {
        int gi = (int)(p9_blur[i] * gsc);
        if (gi < 0) gi = 0;
        if (gi > 1023) gi = 1023;
        int g = p9_glut[gi];
        p9_gl[i] = (uint8_t)g;
        int base = p9_ground[i];
        /* thread hue = glow ramp + radius bias; ground hue = radius only */
        int col = 4096 + p9_hrad[i] + (int)(((0.55f * P9_SCALE) / 255.0f) * (float)g);
        int q = base + (((col - base) * g) >> 8);
        if (q < 0) q = 0;
        if (q > 65535) q = 65535;
        p9_idx[i] = (uint16_t)q;
    }
    uint32_t off = (uint32_t)((double)frame * (0.0005 * (double)P9_SCALE)) & JD_PAL_MASK;
    p9_blit(fb, w, h, pal, off);
}
