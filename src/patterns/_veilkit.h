/* _veilkit.h — tiny shared toolkit for the FIELD-role patterns 202-335.
 *
 * Everything is static: nothing escapes the including translation unit but
 * its own pattern_NNN.  The kit gives each pattern a fast sine, a hashed
 * value-noise / fbm, a colour scaler (multiply towards black = transparency
 * under the compositor's MAX/screen blend), and a small RGB canvas that is
 * bilinear-upscaled to the framebuffer through _upsample.h.
 */
#ifndef VEILKIT_H
#define VEILKIT_H

#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define VK_UNUSED __attribute__((unused))
#define VK_TAU 6.28318530718f
#define VK_SN 4096
#define VK_SMASK 4095

static float vk_sintab[VK_SN];
static int   vk_sin_ready = 0;

VK_UNUSED static void vk_init(void)
{
    if (vk_sin_ready) return;
    for (int i = 0; i < VK_SN; i++)
        vk_sintab[i] = sinf((float)i * (VK_TAU / VK_SN));
    vk_sin_ready = 1;
}

/* Table read, LINEARLY INTERPOLATED (TEMPORAL REVIEW 2.4.0,
 * docs/review/04_pattern_temporal.md, F-VK).  The raw lookup was
 * `tab[(int)(q) & MASK]`: a slow per-frame scalar — a rotation, a drifting
 * centre, a breathing radius — that goes through it moves in STEPS of one
 * table cell every few frames, and when that scalar drives geometry the
 * whole field jerks (measured: 251 star_lattice delta 6.2 spikes every ~140
 * frames on a 0.10 median, 245 orbit_rings kink 2.5, 208/237 alternating
 * 0.1/2.0).  Two taps and a lerp make the table C0-continuous; per-pixel
 * cost is one extra load and a multiply-add. */
VK_UNUSED static inline float vk_tab(float q)
{
    float fl = floorf(q); int i = (int)fl; float f = q - fl;
    float a = vk_sintab[i & VK_SMASK], b = vk_sintab[(i + 1) & VK_SMASK];
    return a + (b - a) * f;
}
/* sine/cosine of a phase in RADIANS (any magnitude) */
VK_UNUSED static inline float vk_sin(float ph)
{
    return vk_tab(ph * (VK_SN / VK_TAU));
}
VK_UNUSED static inline float vk_cos(float ph)
{
    return vk_tab(ph * (VK_SN / VK_TAU) + (float)(VK_SN / 4));
}
/* sine of a phase in TURNS (0..1 = one cycle) */
VK_UNUSED static inline float vk_sint(float t)
{
    return vk_tab(t * VK_SN);
}

VK_UNUSED static inline uint32_t vk_hash(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}
VK_UNUSED static inline float vk_hashf(uint32_t x)          /* 0..1 */
{
    return (float)(vk_hash(x) & 0xFFFFFF) * (1.0f / 16777216.0f);
}
VK_UNUSED static inline float vk_h2(int x, int y, uint32_t s)
{
    return vk_hashf((uint32_t)x * 0x9E3779B1u ^ (uint32_t)y * 0x85EBCA77u ^ s);
}
VK_UNUSED static inline float vk_h3(int x, int y, int z, uint32_t s)
{
    return vk_hashf((uint32_t)x * 0x9E3779B1u ^ (uint32_t)y * 0x85EBCA77u
                    ^ (uint32_t)z * 0xC2B2AE3Du ^ s);
}
VK_UNUSED static inline float vk_smooth(float t) { return t * t * (3.0f - 2.0f * t); }

/* 2-D value noise, 0..1, cell size 1 */
VK_UNUSED static float vk_noise2(float x, float y, uint32_t s)
{
    float fx = floorf(x), fy = floorf(y);
    int ix = (int)fx, iy = (int)fy;
    float tx = vk_smooth(x - fx), ty = vk_smooth(y - fy);
    float a = vk_h2(ix, iy, s),     b = vk_h2(ix + 1, iy, s);
    float c = vk_h2(ix, iy + 1, s), d = vk_h2(ix + 1, iy + 1, s);
    float u = a + (b - a) * tx, v = c + (d - c) * tx;
    return u + (v - u) * ty;
}
/* 3-D value noise (z = slow time), 0..1 */
VK_UNUSED static float vk_noise3(float x, float y, float z, uint32_t s)
{
    float fx = floorf(x), fy = floorf(y), fz = floorf(z);
    int ix = (int)fx, iy = (int)fy, iz = (int)fz;
    float tx = vk_smooth(x - fx), ty = vk_smooth(y - fy), tz = vk_smooth(z - fz);
    float n0, n1;
    {
        float a = vk_h3(ix, iy, iz, s),     b = vk_h3(ix + 1, iy, iz, s);
        float c = vk_h3(ix, iy + 1, iz, s), d = vk_h3(ix + 1, iy + 1, iz, s);
        float u = a + (b - a) * tx, v = c + (d - c) * tx;
        n0 = u + (v - u) * ty;
    }
    {
        float a = vk_h3(ix, iy, iz + 1, s),     b = vk_h3(ix + 1, iy, iz + 1, s);
        float c = vk_h3(ix, iy + 1, iz + 1, s), d = vk_h3(ix + 1, iy + 1, iz + 1, s);
        float u = a + (b - a) * tx, v = c + (d - c) * tx;
        n1 = u + (v - u) * ty;
    }
    return n0 + (n1 - n0) * tz;
}
VK_UNUSED static float vk_fbm2(float x, float y, int oct, uint32_t s)
{
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    for (int i = 0; i < oct; i++) {
        sum += amp * vk_noise2(x, y, s + (uint32_t)i * 977u);
        norm += amp; amp *= 0.5f; x = x * 2.03f + 7.1f; y = y * 1.97f + 3.3f;
    }
    return sum / norm;
}
VK_UNUSED static float vk_fbm3(float x, float y, float z, int oct, uint32_t s)
{
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    for (int i = 0; i < oct; i++) {
        sum += amp * vk_noise3(x, y, z, s + (uint32_t)i * 977u);
        norm += amp; amp *= 0.5f;
        x = x * 2.03f + 7.1f; y = y * 1.97f + 3.3f; z = z * 1.9f + 1.7f;
    }
    return sum / norm;
}

/* smoothstep on [e0,e1] */
VK_UNUSED static inline float vk_sstep(float e0, float e1, float x)
{
    float t = (x - e0) / (e1 - e0);
    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    return t * t * (3.0f - 2.0f * t);
}
VK_UNUSED static inline float vk_clamp01(float x)
{
    return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x;
}
VK_UNUSED static inline float vk_absf(float x) { return x < 0.0f ? -x : x; }
/* triangle wave of x, period 1, 0..1 */
VK_UNUSED static inline float vk_tri(float x)
{
    float f = x - floorf(x);
    return f < 0.5f ? f * 2.0f : 2.0f - f * 2.0f;
}
VK_UNUSED static inline float vk_fract(float x) { return x - floorf(x); }

/* palette sample: u in palette-index units (float, wraps) */
VK_UNUSED static inline uint32_t vk_pal(const uint32_t *pal, float u)
{
    return pal[(int)u & JD_PAL_MASK];
}
/* multiply colour towards black, m in 0..1 */
VK_UNUSED static inline uint32_t vk_scale(uint32_t c, float m)
{
    if (m <= 0.0f) return 0xFF000000u;
    if (m > 1.0f) m = 1.0f;
    unsigned k = (unsigned)(m * 256.0f);
    uint32_t rb = (((c & 0xFF00FFu) * k) >> 8) & 0xFF00FFu;
    uint32_t g  = (((c & 0x00FF00u) * k) >> 8) & 0x00FF00u;
    return 0xFF000000u | rb | g;
}
/* saturating add of two colours */
VK_UNUSED static inline uint32_t vk_add(uint32_t a, uint32_t b)
{
    uint32_t r = ((a >> 16) & 255) + ((b >> 16) & 255);
    uint32_t g = ((a >> 8) & 255) + ((b >> 8) & 255);
    uint32_t bl = (a & 255) + (b & 255);
    if (r > 255) r = 255; if (g > 255) g = 255; if (bl > 255) bl = 255;
    return 0xFF000000u | (r << 16) | (g << 8) | bl;
}
VK_UNUSED static inline uint32_t vk_max(uint32_t a, uint32_t b)
{
    uint32_t r1 = a & 0xFF0000u, r2 = b & 0xFF0000u;
    uint32_t g1 = a & 0x00FF00u, g2 = b & 0x00FF00u;
    uint32_t b1 = a & 0xFFu, b2 = b & 0xFFu;
    return 0xFF000000u | (r1 > r2 ? r1 : r2) | (g1 > g2 ? g1 : g2) | (b1 > b2 ? b1 : b2);
}
VK_UNUSED static inline uint32_t vk_lerp(uint32_t a, uint32_t b, float f)
{
    unsigned k = (unsigned)(vk_clamp01(f) * 256.0f), g = 256u - k;
    uint32_t rb = ((((a & 0xFF00FFu) * g) + ((b & 0xFF00FFu) * k)) >> 8) & 0xFF00FFu;
    uint32_t gg = ((((a & 0x00FF00u) * g) + ((b & 0x00FF00u) * k)) >> 8) & 0x00FF00u;
    return 0xFF000000u | rb | gg;
}
/* palette colour pre-scaled — the everyday call */
VK_UNUSED static inline uint32_t vk_pc(const uint32_t *pal, float u, float m)
{
    return vk_scale(vk_pal(pal, u), m);
}

/* two palette colours blended by f, then scaled: cheap two-tone richness */
VK_UNUSED static inline uint32_t vk_pc2(const uint32_t *pal, float ci, float cj, float f, float m)
{
    return vk_scale(vk_lerp(vk_pal(pal, ci), vk_pal(pal, cj), f), m);
}

/* ---- canvas: render small, blit big --------------------------------- */
typedef struct {
    uint8_t *img;     /* sw*sh*3 RGB */
    int sw, sh;
    jd_up up;
} vk_canvas;

VK_UNUSED static int vk_canvas_prep(vk_canvas *c, int sw, int sh)
{
    if (c->img && c->sw == sw && c->sh == sh) return 1;
    free(c->img);
    c->img = (uint8_t *)calloc((size_t)sw * sh * 3, 1);
    c->sw = sw; c->sh = sh;
    return c->img != NULL;
}
VK_UNUSED static inline void vk_put(vk_canvas *c, int x, int y, uint32_t col)
{
    uint8_t *p = c->img + ((size_t)y * c->sw + x) * 3;
    p[0] = (uint8_t)(col >> 16); p[1] = (uint8_t)(col >> 8); p[2] = (uint8_t)col;
}
VK_UNUSED static inline void vk_putp(uint8_t *p, uint32_t col)
{
    p[0] = (uint8_t)(col >> 16); p[1] = (uint8_t)(col >> 8); p[2] = (uint8_t)col;
}
VK_UNUSED static inline uint32_t vk_getp(const uint8_t *p)
{
    return 0xFF000000u | ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
}
VK_UNUSED static void vk_blit(vk_canvas *c, uint32_t *fb, int w, int h)
{
    jd_up_blit(&c->up, fb, w, h, c->img, c->sw, c->sh);
}
/* choose a canvas size ~ 1/4 of the framebuffer, keeping aspect.
 *
 * PERF REVIEW 2.4.0 (docs/review/06_performance.md, F-VEIL): the canvas is
 * sized from the REAL framebuffer, so on a Retina window (2560x1920 drawable
 * for the default 1280x960 window; 3456x2160 full screen) every pattern in
 * this family rendered a canvas 4x the area it renders at 1280x960 — and the
 * old 640x480 clamp is exactly 2560/4.  Measured (P-core, per layer alone):
 * family median 3.0 ms at 1280x960 -> 8.0 ms at 2560x1920 -> 9.6 ms at
 * 3456x2160, worst 8.5 -> 37.7 -> 43.6 (216_frost_ferns).  Cap the canvas at
 * what a 1280x960 window would get instead: identical output at 1280x960 and
 * below (a = w/div is unchanged there), and on Retina the field is upsampled
 * a little further — these are soft fields by design, the same trade the
 * gk336 grounds already make with a fixed 320x240 canvas. */
VK_UNUSED static void vk_size(int w, int h, int div, int *sw, int *sh)
{
    int a = w / div, b = h / div;
    if (a < 64) a = 64; if (b < 48) b = 48;
    if (a > 1280 / div) a = 1280 / div;     /* Retina: render as if 1280x960 */
    if (b >  960 / div) b =  960 / div;
    if (a > 640) a = 640; if (b > 480) b = 480;
    *sw = a; *sh = b;
}

/* Pick a palette base index for this segment: the palette is scheme-blended
 * and some stretches of it are near-black, so a blind seed offset can land a
 * whole tenancy in the dark.  Score 48 seed-rotated windows of `span`
 * entries by mean luma and take one of the brightest few (seed picks which),
 * so the field is always coloured yet no two segments start the same. */
/* GATE 2.4.0 FIX: the choice below is a RANKING of palette windows, and the
 * palette crossfades every frame — so two near-equal candidates flip-flop and
 * the whole layer pops between two hues (measured in JD_MODE: 218 luma 69<->92
 * eight times in 120 frames, mean channel delta 18 per pop, 134 patterns
 * affected).  seed is stable for the segment, so choose ONCE per seed and hold
 * it; the engine slides the palette pointer for audio rotation, so colour still
 * travels smoothly.  4-entry cache in case a pattern is hosted twice at once. */
VK_UNUSED static float vk_base(const uint32_t *pal, uint32_t seed, int span)
{
    static uint32_t vkb_seed[4]; static float vkb_val[4]; static int vkb_n, vkb_ok[4];
    for (int i = 0; i < 4; i++)
        if (vkb_ok[i] && vkb_seed[i] == seed) return vkb_val[i];
    int   best[6]; float bl[6];
    for (int i = 0; i < 6; i++) { best[i] = 0; bl[i] = -1.0f; }
    int start = (int)(seed & 0x7FFF);
    for (int k = 0; k < 48; k++) {
        int o = (start + k * 683) & JD_PAL_MASK;
        float l = 0.0f;
        for (int j = 0; j < 12; j++) {
            uint32_t c = pal[(o + j * span / 12) & JD_PAL_MASK];
            l += (float)((((c >> 16) & 255) * 77 + ((c >> 8) & 255) * 150 + (c & 255) * 29) >> 8);
        }
        /* insert into the top-6 list */
        for (int i = 0; i < 6; i++) {
            if (l > bl[i]) {
                for (int j = 5; j > i; j--) { bl[j] = bl[j - 1]; best[j] = best[j - 1]; }
                bl[i] = l; best[i] = o; break;
            }
        }
    }
    float r = (float)best[vk_hash(seed ^ 0xB45Eu) % 6u];
    vkb_seed[vkb_n & 3] = seed; vkb_val[vkb_n & 3] = r; vkb_ok[vkb_n & 3] = 1; vkb_n++;
    return r;
}

/* per-seed parameter helpers */
VK_UNUSED static inline float vk_seedf(uint32_t seed, int k) { return vk_hashf(seed + (uint32_t)k * 7919u); }
VK_UNUSED static inline float vk_seedr(uint32_t seed, int k, float lo, float hi)
{
    return lo + (hi - lo) * vk_seedf(seed, k);
}

#endif /* VEILKIT_H */
