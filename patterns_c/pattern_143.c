/* 143 Caustic Pool — the light net on the floor of a swimming pool.
 * A four-wave surface h(x,y,t) is not drawn; what is drawn is where its light
 * *lands*. Every cell of a 320x240 grid launches four photons (2x2 supersample
 * plus a static sub-pixel jitter tile), refracted by the local surface slope
 * to (x - G dh/dx, y - G dh/dy). Each photon carries 1/(0.045 + |det J|) - 3.2
 * clamped at zero, with the Jacobian of the refraction map taken analytically
 * from the same four waves — so only cells sitting on a *fold* of the map emit
 * anything. The fold locus det J = 0 is exactly where real caustics form, so
 * the filaments are thin and bright and the deep water between them stays
 * black. Two sets at 10% different refraction strength stand in for chromatic
 * dispersion: the net gets a warm edge and a cool one. Blurred, tone-mapped
 * and bilinearly upscaled over a dark depth-shaded floor. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define P143_GW 320
#define P143_GH 240
#define P143_N  (P143_GW * P143_GH)
#define P143_NW 4

static float p143_a[P143_N];        /* warm photon energy */
static float p143_b[P143_N];        /* cool photon energy */
static float p143_t1[P143_N];
static uint8_t p143_img[P143_N * 3];
static float p143_cos[4096];
static uint8_t p143_tone[1024];
static uint8_t p143_deep[P143_N];   /* static depth shading of the floor */
static float p143_jx[65536], p143_jy[65536];     /* static launch jitter tile */
static int p143_ready;
static int p143_uw = -1, p143_uh = -1;
static int *p143_xi;
static uint8_t *p143_fx;

static void p143_init(void)
{
    int i, x, y;
    for (i = 0; i < 4096; i++)
        p143_cos[i] = cosf((float)i * (6.28318531f / 4096.0f));
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (1.0f / 150.0f)));
        p143_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    for (y = 0; y < P143_GH; y++)
        for (x = 0; x < P143_GW; x++) {
            float dx = ((float)x - P143_GW * 0.5f) / (P143_GW * 0.5f);
            float dy = ((float)y - P143_GH * 0.5f) / (P143_GH * 0.5f);
            float r = dx * dx + dy * dy * 0.8f;
            float v = 1.0f - 0.55f * r;
            if (v < 0.15f) v = 0.15f;
            p143_deep[y * P143_GW + x] = (uint8_t)(v * 255.0f);
        }
    {
        uint32_t s = 0x1234567u;
        for (i = 0; i < 65536; i++) {
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            p143_jx[i] = (float)((s >> 8) & 1023) * (0.25f / 1024.0f) - 0.125f;
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            p143_jy[i] = (float)((s >> 8) & 1023) * (0.25f / 1024.0f) - 0.125f;
        }
    }
    p143_ready = 1;
}

static inline void p143_splat(float *acc, float fx, float fy, float wt)
{
    int xi = (int)fx, yi = (int)fy;
    if ((unsigned)xi >= P143_GW - 1u || (unsigned)yi >= P143_GH - 1u) return;
    float u = fx - (float)xi, v = fy - (float)yi;
    float *p = acc + (size_t)yi * P143_GW + xi;
    wt *= 0.25f;
    p[0] += (1.0f - u) * (1.0f - v) * wt;
    p[1] += u * (1.0f - v) * wt;
    p[P143_GW] += (1.0f - u) * v * wt;
    p[P143_GW + 1] += u * v * wt;
}

/* separable 1-2-1 blur, in place via scratch */
static void p143_blur(float *src)
{
    int x, y;
    for (y = 0; y < P143_GH; y++) {
        const float *s = src + (size_t)y * P143_GW;
        float *d = p143_t1 + (size_t)y * P143_GW;
        d[0] = (s[0] * 3.0f + s[1]) * 0.25f;
        for (x = 1; x < P143_GW - 1; x++)
            d[x] = (s[x - 1] + s[x] * 2.0f + s[x + 1]) * 0.25f;
        d[x] = (s[x] * 3.0f + s[x - 1]) * 0.25f;
    }
    for (y = 0; y < P143_GH; y++) {
        const float *s0 = p143_t1 + (size_t)(y > 0 ? y - 1 : 0) * P143_GW;
        const float *s1 = p143_t1 + (size_t)y * P143_GW;
        const float *s2 = p143_t1 + (size_t)(y < P143_GH - 1 ? y + 1 : y) * P143_GW;
        float *d = src + (size_t)y * P143_GW;
        for (x = 0; x < P143_GW; x++)
            d[x] = (s0[x] + s1[x] * 2.0f + s2[x]) * 0.25f;
    }
}

static void p143_upscale(uint32_t *fb, int w, int h)
{
    if (w != p143_uw) {
        free(p143_xi); free(p143_fx);
        p143_xi = (int *)malloc(sizeof(int) * (size_t)w);
        p143_fx = (uint8_t *)malloc((size_t)w);
        for (int x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (P143_GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8;
            if (xi > P143_GW - 2) { xi = P143_GW - 2; q = (P143_GW - 1) * 256; }
            p143_xi[x] = xi * 3; p143_fx[x] = (uint8_t)(q & 255);
        }
        p143_uw = w;
    }
    p143_uh = h;
    for (int y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (P143_GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8;
        if (yi > P143_GH - 2) { yi = P143_GH - 2; qy = (P143_GH - 1) * 256; }
        int fy = qy & 255;
        const uint8_t *r0 = p143_img + (size_t)yi * P143_GW * 3;
        const uint8_t *r1 = r0 + P143_GW * 3;
        uint32_t *out = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) {
            int X = p143_xi[x], fx = p143_fx[x], c[3];
            for (int k = 0; k < 3; k++) {
                int t0 = r0[X + k] + (((r0[X + 3 + k] - r0[X + k]) * fx) >> 8);
                int t1 = r1[X + k] + (((r1[X + 3 + k] - r1[X + k]) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16)
                   | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

void pattern_143(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (!p143_ready) p143_init();

    const float t = (float)frame;
    const float sd = (float)(seed & 511u) * 0.012272f;

    /* four gently detuned surface waves; wavelengths 55..140 grid px */
    static const float wl[P143_NW]  = { 48.0f, 32.0f, 66.0f, 39.0f };
    static const float ang[P143_NW] = { 0.35f, 1.72f, 2.55f, 4.31f };
    static const float amp[P143_NW] = { 1.00f, 0.72f, 0.86f, 0.55f };
    static const float spd[P143_NW] = { 0.0280f, -0.0215f, 0.0170f, -0.0332f };

    int stx[P143_NW], sty[P143_NW], ph[P143_NW];
    float dxc[P143_NW], dyc[P143_NW];            /* displacement, pixels */
    float jxx[P143_NW], jyy[P143_NW], jxy[P143_NW];  /* Jacobian terms */
    const float S = 1.25f;                       /* surface steepness d*k */
    for (int i = 0; i < P143_NW; i++) {
        float k = 6.28318531f / wl[i];
        float a = ang[i] + 0.22f * sinf(t * 0.00037f + (float)i * 1.3f + sd);
        float u = cosf(a), v = sinf(a);
        stx[i] = (int)(k * u * (4096.0f / 6.28318531f) * 256.0f);
        sty[i] = (int)(k * v * (4096.0f / 6.28318531f) * 256.0f);
        ph[i]  = (int)((t * spd[i] + sd * (float)(i + 1)) * (4096.0f / 6.28318531f) * 256.0f);
        float A = S * amp[i];
        dxc[i] = A * u / k; dyc[i] = A * v / k;
        jxx[i] = A * u * u;  jyy[i] = A * v * v;  jxy[i] = A * u * v;
    }

    memset(p143_a, 0, sizeof p143_a);
    memset(p143_b, 0, sizeof p143_b);

    /* 2x2 supersampled photon launch. Each photon carries 1/(eps+|det J|):
     * the fold locus det J = 0 is where refracted light actually concentrates,
     * so only cells near a fold emit anything and the floor stays black. */
    int stx2[P143_NW];
    for (int i = 0; i < P143_NW; i++) stx2[i] = stx[i] >> 1;
    for (int sy = 0; sy < 2 * P143_GH; sy++) {
        float fy = (float)sy * 0.5f + 0.25f;
        int acc[P143_NW];
        for (int i = 0; i < P143_NW; i++) acc[i] = ph[i] + (int)(fy * (float)sty[i]);
        const float *jx = p143_jx + ((sy & 255) << 8);
        const float *jy = p143_jy + ((sy & 255) << 8);
        for (int sx = 0; sx < 2 * P143_GW; sx++) {
            int a0 = (acc[0] >> 8) & 4095, a1 = (acc[1] >> 8) & 4095;
            int a2 = (acc[2] >> 8) & 4095, a3 = (acc[3] >> 8) & 4095;
            float c0 = p143_cos[a0], c1 = p143_cos[a1];
            float c2 = p143_cos[a2], c3 = p143_cos[a3];
            float s0 = p143_cos[(a0 + 3072) & 4095], s1 = p143_cos[(a1 + 3072) & 4095];
            float s2 = p143_cos[(a2 + 3072) & 4095], s3 = p143_cos[(a3 + 3072) & 4095];
            acc[0] += stx2[0]; acc[1] += stx2[1]; acc[2] += stx2[2]; acc[3] += stx2[3];
            float dx = dxc[0] * c0 + dxc[1] * c1 + dxc[2] * c2 + dxc[3] * c3;
            float dy = dyc[0] * c0 + dyc[1] * c1 + dyc[2] * c2 + dyc[3] * c3;
            float Jxx = 1.0f - (jxx[0] * s0 + jxx[1] * s1 + jxx[2] * s2 + jxx[3] * s3);
            float Jyy = 1.0f - (jyy[0] * s0 + jyy[1] * s1 + jyy[2] * s2 + jyy[3] * s3);
            float Jxy =       -(jxy[0] * s0 + jxy[1] * s1 + jxy[2] * s2 + jxy[3] * s3);
            float det = Jxx * Jyy - Jxy * Jxy;
            if (det < 0.0f) det = -det;
            float wgt = 1.0f / (0.045f + det) - 3.2f;
            if (wgt <= 0.0f) continue;
            int j = sx & 255;
            float bx = (float)sx * 0.5f + 0.25f + jx[j] - dx, by = fy + jy[j] - dy;
            p143_splat(p143_a, bx, by, wgt);
            p143_splat(p143_b, bx - dx * 0.10f, by - dy * 0.10f, wgt * 0.85f);
        }
    }
    p143_blur(p143_a);
    p143_blur(p143_b);

    const int cidx = (int)(t * 0.8f) + (int)(seed & 4095u);
    uint8_t *o = p143_img;
    for (int i = 0; i < P143_N; i++) {
        int ia = (int)(p143_a[i] * 13.0f); if (ia > 1023) ia = 1023;
        int ib = (int)(p143_b[i] * 13.0f); if (ib > 1023) ib = 1023;
        int va = p143_tone[ia], vb = p143_tone[ib];
        int dp = p143_deep[i];
        uint32_t ca = pal[(uint32_t)(cidx + 5200 + ((va * 2400) >> 8)) & JD_PAL_MASK];
        uint32_t cb = pal[(uint32_t)(cidx + 1400 + ((vb * 1800) >> 8)) & JD_PAL_MASK];
        uint32_t cw = pal[(uint32_t)(cidx + 800) & JD_PAL_MASK];         /* water */
        int wr = (int)((((cw >> 16) & 255u) * (uint32_t)dp) >> 8) >> 3;
        int wg = (int)((((cw >> 8) & 255u) * (uint32_t)dp) >> 8) >> 3;
        int wb = (int)(((cw & 255u) * (uint32_t)dp) >> 8) >> 3;
        int r = wr + (int)((((ca >> 16) & 255u) * (uint32_t)va) >> 8)
                   + (int)((((cb >> 16) & 255u) * (uint32_t)vb) >> 9);
        int g = wg + (int)((((ca >> 8) & 255u) * (uint32_t)va) >> 8)
                   + (int)((((cb >> 8) & 255u) * (uint32_t)vb) >> 9);
        int b = wb + (int)(((ca & 255u) * (uint32_t)va) >> 8)
                   + (int)(((cb & 255u) * (uint32_t)vb) >> 9);
        if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
        *o++ = (uint8_t)r; *o++ = (uint8_t)g; *o++ = (uint8_t)b;
    }
    p143_upscale(fb, w, h);
}
