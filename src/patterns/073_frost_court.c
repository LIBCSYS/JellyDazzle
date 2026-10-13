/* 073 Frost Court — six-fold frost dendrite on a midnight pane.
 * Port of lab/patterns/073_frost_court/proto.py: one +x arm (spine + 60-deg
 * side branches + secondary spurs) replicated by 6 slow rotations, shimmer
 * wave along arclength, hexagonal centre plate, additive splat + exp tonemap.
 * Internal 320x240 canvas, bilinear upscale to (w,h). Repaint pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GW 320
#define GH 240
#define NMAX 4096

static float (*acc)[GW][3];
static float (*acc2)[GW][3];
static uint8_t (*img)[GW][3];
static uint8_t (*bgp)[GW][3];
static float (*bgg)[GW];             /* 3.6.5: pane glow falloff, 0..1 */
static uint8_t tonelut[2048];
static float huetab[1024][3];
static float ax[NMAX], ay[NMAX], ad[NMAX];
static float ac[NMAX][3];
static int napts;
static int inited;
static int up_w = -1;
static int *up_xi;
static uint8_t *up_fx;
/* 3.6.5: seed now picks branch spacing and reach, spur rhythm, rotation
 * phase/direction, a small off-centre nucleation point and the hue; the pane
 * itself is tinted from the palette. It used to be the same dendrite on the
 * same fixed midnight-blue pane (audit dSeed 0.71, dPal 0.61). */
static uint32_t p73_seed = 0xFFFFFFFFu;
static float p73_node = 8.0f, p73_reach = 0.42f, p73_spur = 10.0f;
static float p73_rot0 = 0.0f, p73_rdir = 1.0f, p73_shim = 0.045f;
static float p73_cx = GW / 2.0f, p73_cy = GH / 2.0f, p73_hue = 0.0f;

static uint32_t p73_hash(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}

static void p73_reseed(uint32_t seed) {
    uint32_t a = p73_hash(seed ^ 0x73F0u), b = p73_hash(a);
    p73_node  = 6.0f + (float)(a & 63u) * (5.0f / 63.0f);
    p73_reach = 0.32f + (float)((a >> 6) & 63u) * (0.20f / 63.0f);
    p73_spur  = 7.0f + (float)((a >> 12) & 63u) * (7.0f / 63.0f);
    p73_rot0  = (float)((a >> 18) & 255u) * (1.04719755f / 256.0f);
    p73_rdir  = ((a >> 26) & 1u) ? -1.0f : 1.0f;
    p73_shim  = 0.035f + (float)((a >> 27) & 31u) * (0.025f / 31.0f);
    p73_cx = GW / 2.0f + (float)((int)(b & 63u) - 32) * 0.9f;
    p73_cy = GH / 2.0f + (float)((int)((b >> 6) & 63u) - 32) * 0.6f;
    p73_hue = (float)((b >> 12) & 1023u) * (1.0f / 1024.0f);
    p73_seed = seed;
}

/* pane colour from the palette: a hue from just past the frost's own band,
 * normalised so the brightest channel carries the glow, kept dim so the
 * crystal still reads against it */
static void p73_build_bg(float hue) {
    const float *c = huetab[(int)(hue * 1024.0f) & 1023];
    float k0 = 255.0f * 0.045f, k1 = 255.0f * 0.17f;
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            float v = k0 + k1 * bgg[y][x];
            bgp[y][x][0] = (uint8_t)(c[0] * v);
            bgp[y][x][1] = (uint8_t)(c[1] * v);
            bgp[y][x][2] = (uint8_t)(c[2] * v);
        }
}

static void build_huetab(const uint32_t *pal) {
    for (int i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255), b = (float)(u & 255);
        float m = r > g ? r : g; if (b > m) m = b; if (m < 24.0f) m = 24.0f;
        float n = 1.0f / m;
        huetab[i][0] = r * n; huetab[i][1] = g * n; huetab[i][2] = b * n;
    }
}

/* HSV-alike using the palette as the hue wheel */
static void palcol(float hue, float sat, float val, float *out) {
    int i = (int)(hue * 1024.0f);
    const float *c = huetab[i & 1023];
    float w = 1.0f - sat;
    out[0] = (w + sat * c[0]) * val;
    out[1] = (w + sat * c[1]) * val;
    out[2] = (w + sat * c[2]) * val;
}

static void splat(float x, float y, const float *c, float w) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx = x - (float)xi, fy = y - (float)yi;
    float w00 = (1 - fx) * (1 - fy) * w, w10 = fx * (1 - fy) * w;
    float w01 = (1 - fx) * fy * w, w11 = fx * fy * w;
    if ((unsigned)xi < GW && (unsigned)yi < GH) {
        acc[yi][xi][0] += c[0] * w00; acc[yi][xi][1] += c[1] * w00; acc[yi][xi][2] += c[2] * w00;
    }
    if ((unsigned)(xi + 1) < GW && (unsigned)yi < GH) {
        acc[yi][xi+1][0] += c[0] * w10; acc[yi][xi+1][1] += c[1] * w10; acc[yi][xi+1][2] += c[2] * w10;
    }
    if ((unsigned)xi < GW && (unsigned)(yi + 1) < GH) {
        acc[yi+1][xi][0] += c[0] * w01; acc[yi+1][xi][1] += c[1] * w01; acc[yi+1][xi][2] += c[2] * w01;
    }
    if ((unsigned)(xi + 1) < GW && (unsigned)(yi + 1) < GH) {
        acc[yi+1][xi+1][0] += c[0] * w11; acc[yi+1][xi+1][1] += c[1] * w11; acc[yi+1][xi+1][2] += c[2] * w11;
    }
}

static void soften1(void) {
    for (int y = 0; y < GH; y++) {
        int ym = (y + GH - 1) % GH, yp = (y + 1) % GH;
        for (int x = 0; x < GW; x++) {
            int xm = (x + GW - 1) % GW, xp = (x + 1) % GW;
            for (int k = 0; k < 3; k++)
                acc2[y][x][k] = acc[y][x][k] * 0.5f
                    + 0.125f * (acc[ym][x][k] + acc[yp][x][k]
                                + acc[y][xm][k] + acc[y][xp][k]);
        }
    }
    float (*tmp)[GW][3] = acc; acc = acc2; acc2 = tmp;
}

static void compose(void) {
    const float *a = &acc[0][0][0];
    const uint8_t *b = &bgp[0][0][0];
    uint8_t *o = &img[0][0][0];
    for (int i = 0; i < GW * GH * 3; i++) {
        int ti = (int)(a[i] * 64.0f);
        if (ti > 2047) ti = 2047;
        if (ti < 0) ti = 0;
        int v = b[i] + tonelut[ti];
        o[i] = (uint8_t)(v > 255 ? 255 : v);
    }
}

static void upscale(uint32_t *fb, int w, int h) {
    if (w != up_w) {
        free(up_xi); free(up_fx);
        up_xi = (int *)malloc(sizeof(int) * (size_t)w);
        up_fx = (uint8_t *)malloc((size_t)w);
        for (int x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8; if (xi > GW - 2) { xi = GW - 2; q = (GW - 1) * 256; }
            up_xi[x] = xi * 3; up_fx[x] = (uint8_t)(q & 255);
        }
        up_w = w;
    }
    for (int y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8; if (yi > GH - 2) { yi = GH - 2; qy = (GH - 1) * 256; }
        int fy = qy & 255;
        const uint8_t *r0 = &img[yi][0][0], *r1 = &img[yi + 1][0][0];
        uint32_t *out = fb + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            int X = up_xi[x], fx = up_fx[x];
            int c[3];
            for (int k = 0; k < 3; k++) {
                int t0 = r0[X + k] + (((r0[X + 3 + k] - r0[X + k]) * fx) >> 8);
                int t1 = r1[X + k] + (((r1[X + 3 + k] - r1[X + k]) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

static void init_tables(void) {
    for (int i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-((float)i / 64.0f) * 0.8f));
        tonelut[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    acc  = malloc(sizeof(float) * GH * GW * 3);
    acc2 = malloc(sizeof(float) * GH * GW * 3);
    img  = malloc(GH * GW * 3);
    bgp  = malloc(GH * GW * 3);
    bgg  = malloc(sizeof(float) * GH * GW);
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            float dx = ((float)x - GW / 2.0f) / GW, dy = ((float)y - GH / 2.0f) / GH;
            bgg[y][x] = expf(-(dx * dx + dy * dy) * 5.0f);
        }
}

static void add_pt(float x, float y, float d) {
    if (napts >= NMAX) return;
    ax[napts] = x; ay[napts] = y; ad[napts] = d; napts++;
}

/* one arm along +x: spine, 60-degree side branches, secondary spurs */
static void arm_points(float L) {
    /* 3.6.5: node spacing, side-branch reach and spur rhythm are seeded
     * (p73_node / p73_reach / p73_spur; lab values 8 / 0.42 / 10) */
    const float ca = 0.5f, sa = 0.8660254f;
    napts = 0;
    for (float s = 0.0f; s < L; s += 1.0f) add_pt(s, 0.0f, s);
    for (float node = p73_node; node < L; node += p73_node) {
        float bmax = (108.0f - node) * p73_reach;
        float lb = (L - node) * 0.85f;
        if (lb < 0.0f) lb = 0.0f;
        if (lb > bmax) lb = bmax;
        if (lb < 2.0f) continue;
        for (float u = 0.0f; u < lb; u += 1.0f) {
            add_pt(node + u * ca,  u * sa, node + u);
            add_pt(node + u * ca, -u * sa, node + u);
        }
        for (float n2 = 6.0f; n2 < lb; n2 += p73_spur) {
            float l2 = (lb - n2) * 0.7f;
            if (l2 > 6.0f) l2 = 6.0f;
            if (l2 < 1.5f) continue;
            for (float u2 = 0.0f; u2 < l2; u2 += 1.0f) {
                add_pt(node + n2 * ca + u2,  (n2 * sa + u2 * 0.15f), node + n2 + u2);
                add_pt(node + n2 * ca + u2, -(n2 * sa + u2 * 0.15f), node + n2 + u2);
            }
        }
    }
}

void pattern_073(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!inited) { init_tables(); inited = 1; }
    if (seed != p73_seed) p73_reseed(seed);
    build_huetab(pal);
    float t = (float)frame;
    float T = (float)sl + 250.0f;
    float L = T * 0.16f; if (L > 105.0f) L = 105.0f;
    float hueoff = p73_hue;
    p73_build_bg(0.64f + hueoff);

    arm_points(L);
    for (int i = 0; i < napts; i++) {
        float s = ad[i];
        float shim = 0.62f + 0.38f * sinf(s * 0.22f - t * p73_shim);
        float hue = 0.52f + 0.06f * sinf(s * 0.05f + t * 0.002f) + hueoff;
        /* 3.6.5: floor raised 0.08 -> 0.30 so the core carries the palette
         * hue instead of reading as the same white on every scheme */
        float sat = 0.80f - 0.55f * expf(-s / 18.0f);
        if (sat < 0.30f) sat = 0.30f; if (sat > 0.85f) sat = 0.85f;
        palcol(hue, sat, shim, ac[i]);
    }

    memset(acc, 0, sizeof(float) * GH * GW * 3);
    float rot0 = p73_rot0 + p73_rdir * t * 0.0012f;
    for (int k = 0; k < 6; k++) {
        float a = rot0 + (float)k * 1.04719755f;
        float ca = cosf(a), sa = sinf(a);
        for (int i = 0; i < napts; i++) {
            float X = p73_cx + ax[i] * ca - ay[i] * sa;
            float Y = p73_cy + ax[i] * sa + ay[i] * ca;
            splat(X, Y, ac[i], 1.0f);
        }
    }
    /* centre hexagonal plate */
    {
        float hr = T * 0.05f; if (hr > 13.0f) hr = 13.0f;
        float col[3];
        palcol(0.53f + hueoff, 0.35f, 0.9f, col);
        for (int i = 0; i < 120; i++) {
            float th = (float)i * (6.2831853f / 120.0f);
            float m = fmodf(th + rot0 + 6.2831853f, 1.04719755f) - 0.52359878f;
            float hx = hr / cosf(m);
            splat(p73_cx + hx * cosf(th), p73_cy + hx * sinf(th), col, 1.2f);
        }
    }
    soften1();
    compose();
    upscale(fb, w, h);
}
