/* _gk336.h — GROUND KIT for patterns 336-468 (full-frame backgrounds).
 *
 * Every background in this family renders a small RGB canvas (GK_W x GK_H,
 * default 320x240) and bilinear-upscales it into the framebuffer with the
 * shared _upsample.h blitter.  This header holds the arithmetic every one of
 * them needs — a sine table, hashed gradient noise (2-D and 3-D), fBm, a
 * palette sampler and a couple of packed-RGB helpers — so each pattern file
 * is nothing but its own field function.
 *
 * Everything is static: each translation unit gets a private copy, nothing
 * but pattern_NNN escapes.  Define GK_W / GK_H before including to change
 * the canvas size (heavier fields render smaller).
 */
#ifndef GK336_H
#define GK336_H

#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <string.h>
#include <stdint.h>

#ifndef GK_W
#define GK_W 320
#endif
#ifndef GK_H
#define GK_H 240
#endif
#define GK_N (GK_W * GK_H)

static uint8_t gk_img[GK_N * 3];
static jd_up   gk_up;
static float   gk_sn[4096];
static float   gk_g2x[256], gk_g2y[256];      /* 2-D unit gradients      */
static float   gk_g3x[256], gk_g3y[256], gk_g3z[256];
static uint8_t gk_perm[512];
static int     gk_ready;

/* ---- deterministic hashing --------------------------------------------- */
static inline uint32_t gk_hash(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}
static inline uint32_t gk_hash2(int x, int y, uint32_t s)
{
    return gk_hash((uint32_t)x * 0x9E3779B1u ^ (uint32_t)y * 0x85EBCA77u ^ s);
}
static inline uint32_t gk_hash3(int x, int y, int z, uint32_t s)
{
    return gk_hash((uint32_t)x * 0x9E3779B1u ^ (uint32_t)y * 0x85EBCA77u
                   ^ (uint32_t)z * 0xC2B2AE3Du ^ s);
}
/* seed -> float 0..1, k-th draw */
static inline float gk_sf(uint32_t seed, uint32_t k)
{
    return (float)(gk_hash(seed * 0x9E3779B1u + k * 0x85EBCA77u) >> 8) * (1.0f / 16777216.0f);
}
/* any 32-bit value -> float 0..1 (re-hashed, so shifted/derived inputs are fine) */
static inline float gk_hf(uint32_t hsh) { return (float)(gk_hash(hsh + 0x68E31DA4u) >> 8) * (1.0f / 16777216.0f); }

static void gk_init(void)
{
    if (gk_ready) return;
    for (int i = 0; i < 4096; i++) gk_sn[i] = sinf((float)i * (6.283185307f / 4096.0f));
    for (int i = 0; i < 256; i++) {
        float a = (float)i * (6.283185307f / 256.0f) + 0.37f;
        gk_g2x[i] = cosf(a); gk_g2y[i] = sinf(a);
        uint32_t hh = gk_hash((uint32_t)i * 7919u + 13u);
        float z = (float)(hh & 0xFFFF) / 32767.5f - 1.0f;
        float r = sqrtf(1.0f - z * z), b = (float)(hh >> 16) * (6.283185307f / 65536.0f);
        gk_g3x[i] = r * cosf(b); gk_g3y[i] = r * sinf(b); gk_g3z[i] = z;
    }
    for (int i = 0; i < 256; i++) gk_perm[i] = (uint8_t)i;
    for (int i = 255; i > 0; i--) {
        int j = (int)(gk_hash((uint32_t)i * 2654435761u + 99u) % (uint32_t)(i + 1));
        uint8_t t = gk_perm[i]; gk_perm[i] = gk_perm[j]; gk_perm[j] = t;
    }
    for (int i = 0; i < 256; i++) gk_perm[256 + i] = gk_perm[i];
    gk_ready = 1;
}

/* ---- trig (argument in radians, any magnitude) --------------------------- */
/* Table read, LINEARLY INTERPOLATED (TEMPORAL REVIEW 2.4.0,
 * docs/review/04_pattern_temporal.md, F-GK).  The raw `tab[(int)q & 4095]`
 * makes any slow per-frame scalar (rotation, centre, breathing radius) move
 * in steps of one table cell every few frames — a whole-field jerk when the
 * scalar drives geometry (measured: 407 geodesic_dome kink 1.43 -> 0.12,
 * 384 corduroy 0.95 -> 0.01, 391/393/425/429/386 all ~0.8-1.0 -> ~0.05).
 * Two taps and a lerp; one extra load and a multiply-add per call. */
static inline float gk_tab(float q)
{
    float fl = floorf(q); int i = (int)fl; float f = q - fl;
    float a = gk_sn[i & 4095], b = gk_sn[(i + 1) & 4095];
    return a + (b - a) * f;
}
static inline float gk_sin(float a) { return gk_tab(a * 651.8986469f); }
static inline float gk_cos(float a) { return gk_tab(a * 651.8986469f + 1024.0f); }
/* argument in TURNS (0..1 = one cycle) */
static inline float gk_sint(float t) { return gk_tab(t * 4096.0f); }
static inline float gk_cost(float t) { return gk_tab(t * 4096.0f + 1024.0f); }

static inline float gk_fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
static inline float gk_lerp(float a, float b, float t) { return a + (b - a) * t; }
static inline float gk_clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }
static inline float gk_sstep(float e0, float e1, float x)
{
    float t = gk_clamp01((x - e0) / (e1 - e0)); return t * t * (3.0f - 2.0f * t);
}
static inline float gk_absf(float v) { return v < 0.0f ? -v : v; }
static inline float gk_fract(float v) { return v - floorf(v); }

/* ---- gradient noise: 2-D, range about -1..1 ----------------------------- */
static inline float gk_n2(float x, float y)
{
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx = x - (float)xi, fy = y - (float)yi;
    int X = xi & 255, Y = yi & 255;
    int a = gk_perm[X] + Y, b = gk_perm[X + 1] + Y;
    int g00 = gk_perm[a], g10 = gk_perm[b], g01 = gk_perm[a + 1], g11 = gk_perm[b + 1];
    float n00 = gk_g2x[g00] * fx        + gk_g2y[g00] * fy;
    float n10 = gk_g2x[g10] * (fx - 1)  + gk_g2y[g10] * fy;
    float n01 = gk_g2x[g01] * fx        + gk_g2y[g01] * (fy - 1);
    float n11 = gk_g2x[g11] * (fx - 1)  + gk_g2y[g11] * (fy - 1);
    float u = gk_fade(fx), v = gk_fade(fy);
    return 1.41f * gk_lerp(gk_lerp(n00, n10, u), gk_lerp(n01, n11, u), v);
}
/* 3-D (x, y, time) */
static inline float gk_n3(float x, float y, float z)
{
    int xi = (int)floorf(x), yi = (int)floorf(y), zi = (int)floorf(z);
    float fx = x - (float)xi, fy = y - (float)yi, fz = z - (float)zi;
    int X = xi & 255, Y = yi & 255, Z = zi & 255;
    int A = gk_perm[X] + Y, AA = gk_perm[A] + Z, AB = gk_perm[A + 1] + Z;
    int B = gk_perm[X + 1] + Y, BA = gk_perm[B] + Z, BB = gk_perm[B + 1] + Z;
    int g000 = gk_perm[AA], g100 = gk_perm[BA], g010 = gk_perm[AB], g110 = gk_perm[BB];
    int g001 = gk_perm[AA + 1], g101 = gk_perm[BA + 1], g011 = gk_perm[AB + 1], g111 = gk_perm[BB + 1];
    float u = gk_fade(fx), v = gk_fade(fy), w = gk_fade(fz);
    float n000 = gk_g3x[g000]*fx     + gk_g3y[g000]*fy     + gk_g3z[g000]*fz;
    float n100 = gk_g3x[g100]*(fx-1) + gk_g3y[g100]*fy     + gk_g3z[g100]*fz;
    float n010 = gk_g3x[g010]*fx     + gk_g3y[g010]*(fy-1) + gk_g3z[g010]*fz;
    float n110 = gk_g3x[g110]*(fx-1) + gk_g3y[g110]*(fy-1) + gk_g3z[g110]*fz;
    float n001 = gk_g3x[g001]*fx     + gk_g3y[g001]*fy     + gk_g3z[g001]*(fz-1);
    float n101 = gk_g3x[g101]*(fx-1) + gk_g3y[g101]*fy     + gk_g3z[g101]*(fz-1);
    float n011 = gk_g3x[g011]*fx     + gk_g3y[g011]*(fy-1) + gk_g3z[g011]*(fz-1);
    float n111 = gk_g3x[g111]*(fx-1) + gk_g3y[g111]*(fy-1) + gk_g3z[g111]*(fz-1);
    float x00 = gk_lerp(n000, n100, u), x10 = gk_lerp(n010, n110, u);
    float x01 = gk_lerp(n001, n101, u), x11 = gk_lerp(n011, n111, u);
    return 1.15f * gk_lerp(gk_lerp(x00, x10, v), gk_lerp(x01, x11, v), w);
}
static inline float gk_fbm2(float x, float y, int oct)
{
    float s = 0, a = 0.5f, n = 0;
    for (int i = 0; i < oct; i++) { s += a * gk_n2(x, y); n += a; a *= 0.5f; x = x * 2.03f + 7.1f; y = y * 1.97f + 3.3f; }
    return s / n;
}
static inline float gk_fbm3(float x, float y, float z, int oct)
{
    float s = 0, a = 0.5f, n = 0;
    for (int i = 0; i < oct; i++) { s += a * gk_n3(x, y, z); n += a; a *= 0.5f; x = x * 2.03f + 7.1f; y = y * 1.97f + 3.3f; z = z * 1.9f + 1.7f; }
    return s / n;
}
/* ridged: sharp bright creases, 0..1 */
static inline float gk_ridge3(float x, float y, float z, int oct)
{
    float s = 0, a = 0.5f, n = 0;
    for (int i = 0; i < oct; i++) {
        float v = 1.0f - gk_absf(gk_n3(x, y, z)); v *= v;
        s += a * v; n += a; a *= 0.5f; x = x * 2.03f + 7.1f; y = y * 1.97f + 3.3f; z = z * 1.9f + 1.7f;
    }
    return s / n;
}

/* ---- colour ---------------------------------------------------------------- */
/* A ground has to EMIT LIGHT (the probe demotes anything under luma 55), but
 * every ramp has dark stretches, and a background that happens to sit in one
 * for a whole segment is a dead screen.  So the palette is lifted per entry:
 * any entry darker than GK_LFLOOR is scaled up (hue-preserving) toward the
 * floor.  Recomputed each frame (32768 cheap ops) because the ramp crossfades. */
#ifndef GK_LFLOOR
#define GK_LFLOOR 80
#endif
static uint32_t gk_lut[32768];              /* lifted copy of the ramp     */
static uint16_t gk_lut_q8[32768 * 3];       /* EWMA state, 8.8 per channel */
static int      gk_lut_ok;
/* GATE 2.4.0 FIX (measured, see docs/GATE_2_4_0.md): the lift is a GAIN of up
 * to 8x on dark ramp entries, so it turns the ramp's 1-unit crossfade steps
 * into 8-unit steps in the image — 351/372/396 read mean channel delta 8.7-10.2
 * on a single palette step (raw ramp step 0.37, lifted 2.9 averaged over the
 * whole ramp).  The original code also skipped the rebuild behind a 5-entry
 * key, so ~150 frames of drift landed at once.  Now the lifted target is
 * computed every frame and the LUT FOLLOWS it through a per-entry EWMA
 * (1/8 per frame): a step becomes an eight-frame glide, and the ramp's own
 * crossfade is unaffected because it is already slow.  ~60 us per call. */
static void gk_prep(const uint32_t *pal)
{
    for (int i = 0; i < 32768; i++) {
        uint32_t c = pal[i];
        uint32_t r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
        uint32_t l = (r * 77 + g * 150 + b * 29) >> 8;
        if (l < GK_LFLOOR) {
            /* hue-preserving gain first ... */
            uint32_t gain = (GK_LFLOOR * 32) / (l + 4); if (gain > 255) gain = 255;
            r = (r * gain) >> 5; g = (g * gain) >> 5; b = (b * gain) >> 5;
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            l = (r * 77 + g * 150 + b * 29) >> 8;
            /* ... then, if it is still dark (pure blues), ease toward white */
            if (l < GK_LFLOOR) {
                uint32_t f = ((GK_LFLOOR - l) * 200) / GK_LFLOOR;      /* Q8, <= 200 */
                r += ((255 - r) * f) >> 8; g += ((255 - g) * f) >> 8; b += ((255 - b) * f) >> 8;
            }
        }
        uint16_t *q = gk_lut_q8 + i * 3;
        if (gk_lut_ok) {
            /* signed EWMA in 8.8: cur += (target - cur) / 8, always moving */
            int tr = (int)(r << 8), tg = (int)(g << 8), tb = (int)(b << 8);
            int dr = tr - q[0], dg = tg - q[1], db = tb - q[2];
            q[0] = (uint16_t)(q[0] + (dr >= 0 ? (dr + 7) >> 3 : -((-dr + 7) >> 3)));
            q[1] = (uint16_t)(q[1] + (dg >= 0 ? (dg + 7) >> 3 : -((-dg + 7) >> 3)));
            q[2] = (uint16_t)(q[2] + (db >= 0 ? (db + 7) >> 3 : -((-db + 7) >> 3)));
        } else {
            q[0] = (uint16_t)(r << 8); q[1] = (uint16_t)(g << 8); q[2] = (uint16_t)(b << 8);
        }
        gk_lut[i] = ((uint32_t)(q[0] >> 8) << 16) | ((uint32_t)(q[1] >> 8) << 8) | (uint32_t)(q[2] >> 8);
    }
    gk_lut_ok = 1;
}
/* palette sample: u in turns (any float, wraps) — lifted */
static inline uint32_t gk_pal(const uint32_t *pal, float u)
{
    (void)pal;
    return gk_lut[(int)(u * 32768.0f) & JD_PAL_MASK];
}
/* raw palette sample, no lift */
static inline uint32_t gk_palraw(const uint32_t *pal, float u)
{
    return pal[(int)(u * 32768.0f) & JD_PAL_MASK] & 0x00FFFFFFu;
}
static inline uint32_t gk_shade(uint32_t c, float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    uint32_t iv = (uint32_t)(v * 256.0f);
    uint32_t rb = ((c & 0x00FF00FFu) * iv >> 8) & 0x00FF00FFu;
    uint32_t g  = ((c & 0x0000FF00u) * iv >> 8) & 0x0000FF00u;
    return rb | g;
}
static inline uint32_t gk_mix(uint32_t a, uint32_t b, float f)
{
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    uint32_t w = (uint32_t)(f * 256.0f), iw = 256u - w;
    uint32_t rb = (((a & 0x00FF00FFu) * iw + (b & 0x00FF00FFu) * w) >> 8) & 0x00FF00FFu;
    uint32_t g  = (((a & 0x0000FF00u) * iw + (b & 0x0000FF00u) * w) >> 8) & 0x0000FF00u;
    return rb | g;
}
/* additive with saturation */
static inline uint32_t gk_add(uint32_t a, uint32_t b)
{
    uint32_t r = ((a >> 16) & 255) + ((b >> 16) & 255);
    uint32_t g = ((a >> 8) & 255) + ((b >> 8) & 255);
    uint32_t bl = (a & 255) + (b & 255);
    if (r > 255) r = 255; if (g > 255) g = 255; if (bl > 255) bl = 255;
    return (r << 16) | (g << 8) | bl;
}
/* lift toward white by f (0..1) — specular / highlight */
static inline uint32_t gk_lift(uint32_t c, float f)
{
    return gk_mix(c, 0x00FFFFFFu, f);
}
static inline void gk_put(int i, uint32_t c)
{
    uint8_t *p = gk_img + i * 3;
    p[0] = (uint8_t)(c >> 16); p[1] = (uint8_t)(c >> 8); p[2] = (uint8_t)c;
}
static inline void gk_putf(int i, float r, float g, float b)
{
    uint8_t *p = gk_img + i * 3;
    p[0] = (uint8_t)(gk_clamp01(r) * 255.0f);
    p[1] = (uint8_t)(gk_clamp01(g) * 255.0f);
    p[2] = (uint8_t)(gk_clamp01(b) * 255.0f);
}
static inline uint32_t gk_get(int i)
{
    const uint8_t *p = gk_img + i * 3;
    return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
}
/* palette colour shaded by v (0..1) — the workhorse */
static inline void gk_pix(int i, const uint32_t *pal, float u, float v)
{
    gk_put(i, gk_shade(gk_pal(pal, u), v));
}

/* call at the top of every pattern: tables + palette lift */
static inline void gk_begin(const uint32_t *pal) { gk_init(); gk_prep(pal); }

/* ---- canvas -> framebuffer ------------------------------------------------ */
static inline void gk_blit(uint32_t *fb, int w, int h)
{
    jd_up_blit(&gk_up, fb, w, h, gk_img, GK_W, GK_H);
}

/* one-pass box blur of the canvas (radius 1), softens hard cells cheaply */
static void gk_soften(void) __attribute__((unused));
static void gk_soften(void)
{
    static uint8_t tmp[GK_N * 3];
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            int x0 = x ? x - 1 : 0, x1 = x < GK_W - 1 ? x + 1 : x;
            for (int c = 0; c < 3; c++)
                tmp[(y * GK_W + x) * 3 + c] = (uint8_t)((gk_img[(y * GK_W + x0) * 3 + c]
                    + 2 * gk_img[(y * GK_W + x) * 3 + c] + gk_img[(y * GK_W + x1) * 3 + c]) >> 2);
        }
    }
    for (int y = 0; y < GK_H; y++) {
        int y0 = y ? y - 1 : 0, y1 = y < GK_H - 1 ? y + 1 : y;
        for (int x = 0; x < GK_W; x++)
            for (int c = 0; c < 3; c++)
                gk_img[(y * GK_W + x) * 3 + c] = (uint8_t)((tmp[(y0 * GK_W + x) * 3 + c]
                    + 2 * tmp[(y * GK_W + x) * 3 + c] + tmp[(y1 * GK_W + x) * 3 + c]) >> 2);
    }
}

#endif /* GK336_H */
