/* 054 Starfield Warp — hue-ringed hyperspace drift with wandering vanishing point.
 * Port of lab/patterns/054_starfield_warp/proto.py.
 * Low-res float accumulator repainted every frame, bilinear upscale to fb. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
static jd_up p054_up;

#define P54_LW 320
#define P54_LH 240
#define P54_CX 160.0f
#define P54_CY 120.0f
#define P54_TAU 6.28318530717958647692f

static float p54_acc[P54_LW * P54_LH * 3];
static unsigned char p54_img[P54_LW * P54_LH * 3];
static int *p54_xmap;
static int p54_xmap_w;

static void p54_splat(float x, float y, float r, float g, float b, float w)
{
    static const int dxs[5] = {0, 1, -1, 0, 0};
    static const int dys[5] = {0, 0, 0, 1, -1};
    static const float ks[5] = {0.85f, 0.30f, 0.30f, 0.30f, 0.30f};
    int i;
    for (i = 0; i < 5; i++) {
        int xi = (int)floorf(x + (float)dxs[i] + 0.5f);
        int yi = (int)floorf(y + (float)dys[i] + 0.5f);
        if ((unsigned)xi < P54_LW && (unsigned)yi < P54_LH) {
            float *p = p54_acc + (yi * P54_LW + xi) * 3;
            float k = ks[i] * w;
            p[0] += r * k; p[1] += g * k; p[2] += b * k;
        }
    }
}

/* 3.6.5: colour straight off the palette ramp. It used to be 65% a fixed HSV
 * rainbow with the palette only tinting it, so pressing C barely changed the
 * stars (variety audit dPal 1.7). hue now walks the whole 32768-entry ramp;
 * the sample is normalised to full chroma so a dark stretch of the ramp
 * still draws a visible star, and sat blends it toward white as before. */
static void p54_color(const uint32_t *pal, float hue, float sat, float val,
                      float *r, float *g, float *b)
{
    float vr, vg, vb, mx;
    uint32_t p;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    vr = (float)((p >> 16) & 255); vg = (float)((p >> 8) & 255); vb = (float)(p & 255);
    mx = vr > vg ? vr : vg; if (vb > mx) mx = vb;
    if (mx < 1.0f) { vr = vg = vb = 1.0f; mx = 1.0f; }
    vr /= mx; vg /= mx; vb /= mx;
    *r = val * ((1.0f - sat) + sat * vr);
    *g = val * ((1.0f - sat) + sat * vg);
    *b = val * ((1.0f - sat) + sat * vb);
}

static void p54_blit(uint32_t *fb, int w, int h)
{
    int i, x;
    int n = P54_LW * P54_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p54_acc[i] * 255.0f;
        p54_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    if (p54_xmap_w != w) {
        free(p54_xmap);
        p54_xmap = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p54_xmap[x] = (int)(((long long)x * (P54_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p54_xmap_w = w;
    }
    jd_up_blit(&p054_up, fb, w, h, p54_img, P54_LW, P54_LH);
}

/* ------------- pattern state ------------- */
#define P54_N 750
#define P54_NS 10
#define P54_ZMAX 1.1f
#define P54_V 0.0022f
#define P54_F 170.0f

static int p54_init_done;

/* 3.6.5: per-appearance flight — the seed used to be ignored, so every
 * appearance was the same star field on the same course (variety audit).
 * The seed now scatters the stars, sets warp speed, how far and in which
 * phase the vanishing point wanders, where the nebula lobes sit, and where
 * on the ramp the star hues start. */
static uint32_t p54_seed = 0xFFFFFFFFu;
static float p54_v = P54_V, p54_wx = 16.0f, p54_wy = 11.0f, p54_ph = 0.0f;
static float p54_h0, p54_na = 0.0f, p54_nb = 0.0f;
static uint32_t p54_h(uint32_t x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }
static float p54_ux[P54_N], p54_uy[P54_N], p54_z0[P54_N];
static float p54_tw[P54_N], p54_hue[P54_N];
/* nebula ground: two gaussian lobes, precomputed unit fields */
static float p54_n1[P54_LW * P54_LH], p54_n2[P54_LW * P54_LH];
static float p54_neb_a = -1e9f, p54_neb_b = -1e9f;

static uint32_t p54_rs;
static float p54_rf(void)
{
    p54_rs ^= p54_rs << 13; p54_rs ^= p54_rs >> 17; p54_rs ^= p54_rs << 5;
    return (float)(p54_rs >> 8) * (1.0f / 16777216.0f);
}
/* box-muller normal */
static float p54_rn(float sd)
{
    float u1 = p54_rf(), u2 = p54_rf();
    if (u1 < 1e-6f) u1 = 1e-6f;
    return sd * sqrtf(-2.0f * logf(u1)) * cosf(P54_TAU * u2);
}

static void p54_init(uint32_t seed)
{
    int i;
    uint32_t a = p54_h(seed ^ 0x5454u), b = p54_h(a), c = p54_h(b);
    p54_v  = 0.0016f + (float)(a % 1000u) * 0.0000012f;         /* 0.0016..0.0028 */
    p54_wx = 8.0f + (float)((a >> 10) % 1000u) * 0.020f;          /* 8..28 px */
    p54_wy = 6.0f + (float)((a >> 20) % 1000u) * 0.012f;          /* 6..18 px */
    p54_ph = (float)((b >> 4) % 1000u) * (P54_TAU / 1000.0f);
    p54_na = -50.0f + (float)((b >> 16) % 1000u) * 0.10f;         /* +-50 px */
    p54_nb = -50.0f + (float)((c >> 16) % 1000u) * 0.10f;
    p54_h0 = (float)((c >> 4) % 1000u) * 0.001f;
    p54_neb_a = p54_neb_b = -1e9f;        /* lobes moved: force a rebuild */
    p54_rs = 0x54D1E205u ^ (c | 1u);
    for (i = 0; i < P54_N; i++) {
        p54_ux[i] = p54_rn(0.50f);
        p54_uy[i] = p54_rn(0.38f);
        p54_z0[i] = p54_rf() * 1.1f;
        p54_tw[i] = p54_rf() * P54_TAU;
        p54_hue[i] = atan2f(p54_uy[i], p54_ux[i]) * (1.0f / P54_TAU)
                     + p54_z0[i] * 0.12f;
    }
    p54_init_done = 1;
    p54_seed = seed;
}

/* rebuild the nebula lobes only when their drift has moved appreciably */
static void p54_nebula(float a, float b)
{
    int x, y;
    if (fabsf(a - p54_neb_a) < 0.35f && fabsf(b - p54_neb_b) < 0.35f) return;
    p54_neb_a = a; p54_neb_b = b;
    for (y = 0; y < P54_LH; y++) {
        float fy = (float)y;
        float ay = (fy - 70.0f) * (1.0f / 95.0f);
        float by = (fy - 180.0f) * (1.0f / 100.0f);
        float ay2 = ay * ay, by2 = by * by;
        for (x = 0; x < P54_LW; x++) {
            float ax = ((float)x - a) * (1.0f / 130.0f);
            float bx = ((float)x - b) * (1.0f / 120.0f);
            p54_n1[y * P54_LW + x] = expf(-(ax * ax + ay2));
            p54_n2[y * P54_LW + x] = expf(-(bx * bx + by2));
        }
    }
}

void pattern_054(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float tt = (float)(frame % 1048576) + 500.0f;
    float cx, cy, hshift;
    float l1r, l1g, l1b, l2r, l2g, l2b;
    int i, si, n;
    (void)sl;
    if (!p54_init_done || seed != p54_seed) p54_init(seed);

    p54_nebula(90.0f + p54_na + 30.0f * sinf(tt * 0.002f + p54_ph),
               240.0f + p54_nb - 25.0f * sinf(tt * 0.0016f + 2.0f));

    /* 3.6.5: nebula lobes tinted from the ramp too (they were fixed violet and
     * teal, and they cover most of the frame). Same per-lobe brightness as the
     * old constants: 0.16 and 0.13 peak. Ramp points drift slowly, no jumps. */
    hshift = tt * 0.0005f + p54_h0;
    p54_color(pal, hshift + 0.25f, 0.9f, 0.16f, &l1r, &l1g, &l1b);
    p54_color(pal, hshift + 0.65f, 0.9f, 0.13f, &l2r, &l2g, &l2b);
    /* a green or gold lobe at the old peak reads twice as bright as the old
     * violet one did; cap each lobe at the old lobe's luminance */
    {
        float L1 = 0.21f * l1r + 0.72f * l1g + 0.07f * l1b;
        float L2 = 0.21f * l2r + 0.72f * l2g + 0.07f * l2b;
        if (L1 > 0.047f) { float k = 0.047f / L1; l1r *= k; l1g *= k; l1b *= k; }
        if (L2 > 0.057f) { float k = 0.057f / L2; l2r *= k; l2g *= k; l2b *= k; }
    }
    n = P54_LW * P54_LH;
    for (i = 0; i < n; i++) {
        float a = p54_n1[i], b = p54_n2[i];
        p54_acc[i * 3 + 0] = l1r * a + l2r * b;
        p54_acc[i * 3 + 1] = l1g * a + l2g * b;
        p54_acc[i * 3 + 2] = l1b * a + l2b * b + 0.03f;
    }

    cx = P54_CX + p54_wx * sinf(tt * 0.0045f + p54_ph);
    cy = P54_CY + p54_wy * sinf(tt * 0.0031f + 1.2f + p54_ph);

    for (i = 0; i < P54_N; i++) {
        float z = p54_z0[i] - p54_v * tt;
        float zp, cr, cg, cb, tk;
        z = fmodf(z, P54_ZMAX);
        if (z < 0.0f) z += P54_ZMAX;
        z += 0.07f;
        zp = z + p54_v * 9.0f;
        tk = 0.8f + 0.2f * sinf(tt * 0.05f + p54_tw[i]);
        for (si = 0; si < P54_NS; si++) {
            float f = (float)si * (1.0f / (float)(P54_NS - 1));
            float zi = z * (1.0f - f) + zp * f;
            float inv = 1.0f / zi;
            float px = cx + p54_ux[i] * P54_F * inv;
            float py = cy + p54_uy[i] * P54_F * inv;
            float bright, sat;
            if (px < -2.0f || px > P54_LW + 2.0f ||
                py < -2.0f || py > P54_LH + 2.0f) continue;
            bright = 0.34f * inv;
            if (bright < 0.05f) bright = 0.05f;
            else if (bright > 1.3f) bright = 1.3f;
            bright *= (1.0f - 0.70f * f) * tk;
            sat = zi * 1.15f;
            if (sat < 0.15f) sat = 0.15f; else if (sat > 0.8f) sat = 0.8f;
            p54_color(pal, p54_hue[i] + hshift, sat, 1.0f, &cr, &cg, &cb);
            p54_splat(px, py, cr, cg, cb, bright);
        }
    }
    p54_blit(fb, w, h);
}
