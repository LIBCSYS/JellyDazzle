/* 132 Tesseract Drift — 4D wireframe polytopes turning inside out.
 *
 * A tesseract (16 vertices, 32 edges) and a counter-rotating 16-cell
 * (8 vertices, 24 edges) are spun by TWO simultaneous 4D plane rotations --
 * one in the xy plane, one in the zw plane, at incommensurate slow rates --
 * which is the isoclinic turn that makes a hypercube appear to evert through
 * itself.  Each vertex is then perspective-projected 4D->3D on w and 3D->2D
 * on z, so edges near the 4-viewer swell and brighten while far ones shrink
 * into the core.  Hue is locked to the 4D w coordinate, so depth in the
 * fourth dimension reads directly as colour and the eye can follow a face
 * around the eversion.  Edges are additively rasterised as filaments into a
 * 384x288 float canvas, bloomed, and bilinearly upscaled: pure glowing line
 * art on black, ~95% near-black, an ideal overlay layer. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AW 384
#define AH 288
#define AN (AW * AH)
#define NV 24                     /* 16 tesseract + 8 cross-polytope */
#define NE 56                     /* 32 + 24                          */

static float p132_acc[AN * 3];
static float p132_blur[AN * 3];
static uint8_t p132_img[AN * 3];
static uint8_t p132_tone[2048];
static float p132_hue[512][3];
static int p132_ready;
static int p132_uw = -1;
static int *p132_uxi;
static uint8_t *p132_ufx;

static float p132_v[NV][4];
static short p132_e[NE][2];
static float p132_px[NV], p132_py[NV], p132_pw[NV], p132_pz[NV];

static void p132_geom(void)
{
    int i, j, k = 0, n = 0;
    /* tesseract: (+-1)^4 scaled */
    for (i = 0; i < 16; i++) {
        p132_v[i][0] = (i & 1) ? 0.62f : -0.62f;
        p132_v[i][1] = (i & 2) ? 0.62f : -0.62f;
        p132_v[i][2] = (i & 4) ? 0.62f : -0.62f;
        p132_v[i][3] = (i & 8) ? 0.62f : -0.62f;
    }
    for (i = 0; i < 16; i++)
        for (j = 0; j < 4; j++)
            if (!(i & (1 << j))) { p132_e[k][0] = (short)i;
                                   p132_e[k][1] = (short)(i | (1 << j)); k++; }
    /* 16-cell: +-e_i, all pairs except antipodal */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) { p132_v[16 + i * 2][j] = 0.0f;
                                  p132_v[17 + i * 2][j] = 0.0f; }
        p132_v[16 + i * 2][i] = 1.02f;
        p132_v[17 + i * 2][i] = -1.02f;
    }
    for (i = 0; i < 8; i++)
        for (j = i + 1; j < 8; j++)
            if ((i >> 1) != (j >> 1)) { p132_e[k][0] = (short)(16 + i);
                                        p132_e[k][1] = (short)(16 + j); k++; }
    n = k; (void)n;
}

static void p132_tabs(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (5.2f / 2048.0f)));
        p132_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    p132_geom();
    p132_ready = 1;
}

static void p132_build_hue(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 512; i++) {
        uint32_t u = pal[(i << 6) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255);
        float g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float m = r > g ? r : g;
        if (b > m) m = b;
        if (m < 30.0f) m = 30.0f;
        m = 1.0f / m;
        p132_hue[i][0] = 0.10f + 0.90f * r * m;
        p132_hue[i][1] = 0.10f + 0.90f * g * m;
        p132_hue[i][2] = 0.10f + 0.90f * b * m;
    }
}

static void p132_splat(float x, float y, const float *c, float wgt)
{
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx, fy, w00, w10, w01, w11;
    if ((unsigned)xi >= AW - 1 || (unsigned)yi >= AH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy) * wgt; w10 = fx * (1.0f - fy) * wgt;
    w01 = (1.0f - fx) * fy * wgt;          w11 = fx * fy * wgt;
    {
        float *a = p132_acc + ((size_t)yi * AW + xi) * 3;
        float *b = a + 3, *d = a + AW * 3, *e = d + 3;
        a[0] += c[0] * w00; a[1] += c[1] * w00; a[2] += c[2] * w00;
        b[0] += c[0] * w10; b[1] += c[1] * w10; b[2] += c[2] * w10;
        d[0] += c[0] * w01; d[1] += c[1] * w01; d[2] += c[2] * w01;
        e[0] += c[0] * w11; e[1] += c[1] * w11; e[2] += c[2] * w11;
    }
}

static void p132_line(float x0, float y0, float x1, float y1,
                      const float *c0, const float *c1, float b0, float b1)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n, i;
    if (len < 0.5f) return;
    n = (int)(len * 1.6f) + 2;
    if (n > 900) n = 900;
    for (i = 0; i <= n; i++) {
        float u = (float)i / (float)n, iu = 1.0f - u;
        float c[3], b = b0 * iu + b1 * u;
        c[0] = (c0[0] * iu + c1[0] * u) * b;
        c[1] = (c0[1] * iu + c1[1] * u) * b;
        c[2] = (c0[2] * iu + c1[2] * u) * b;
        p132_splat(x0 + dx * u, y0 + dy * u, c, 0.62f / 1.6f);
    }
}

static void p132_bloom(void)
{
    int x, y, k;
    for (y = 0; y < AH; y++) {
        float *row = p132_acc + (size_t)y * AW * 3;
        float *out = p132_blur + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < AW - 1 ? x + 1 : AW - 1;
            for (k = 0; k < 3; k++)
                out[x * 3 + k] = 0.5f * row[x * 3 + k]
                               + 0.25f * (row[xm * 3 + k] + row[xp * 3 + k]);
        }
    }
    for (y = 0; y < AH; y++) {
        int ym = y > 0 ? y - 1 : 0, yp = y < AH - 1 ? y + 1 : AH - 1;
        const float *a = p132_blur + (size_t)ym * AW * 3;
        const float *b = p132_blur + (size_t)y * AW * 3;
        const float *c = p132_blur + (size_t)yp * AW * 3;
        float *out = p132_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW * 3; x++)
            out[x] += 0.85f * (0.5f * b[x] + 0.25f * (a[x] + c[x]));
    }
}

static void p132_resolve(void)
{
    int i;
    for (i = 0; i < AN * 3; i++) {
        int t = (int)(p132_acc[i] * 620.0f);
        if (t < 0) t = 0;
        if (t > 2047) t = 2047;
        p132_img[i] = p132_tone[t];
    }
}

static void p132_upscale(uint32_t *fb, int w, int h)
{
    int x, y, k;
    if (w != p132_uw) {
        free(p132_uxi); free(p132_ufx);
        p132_uxi = (int *)malloc(sizeof(int) * (size_t)w);
        p132_ufx = (uint8_t *)malloc((size_t)w);
        for (x = 0; x < w; x++) {
            long long q = ((long long)x * (AW - 1) * 256) / (w > 1 ? w - 1 : 1);
            int xi = (int)(q >> 8);
            if (xi > AW - 2) { xi = AW - 2; q = (long long)(AW - 1) * 256; }
            p132_uxi[x] = xi * 3;
            p132_ufx[x] = (uint8_t)(q & 255);
        }
        p132_uw = w;
    }
    for (y = 0; y < h; y++) {
        long long qy = ((long long)y * (AH - 1) * 256) / (h > 1 ? h - 1 : 1);
        int yi = (int)(qy >> 8), fy;
        const uint8_t *r0, *r1;
        uint32_t *out;
        if (yi > AH - 2) { yi = AH - 2; qy = (long long)(AH - 1) * 256; }
        fy = (int)(qy & 255);
        r0 = p132_img + (size_t)yi * AW * 3; r1 = r0 + AW * 3;
        out = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int X = p132_uxi[x], fx = p132_ufx[x], c[3];
            for (k = 0; k < 3; k++) {
                int t0 = r0[X + k] + (((r0[X + 3 + k] - r0[X + k]) * fx) >> 8);
                int t1 = r1[X + k] + (((r1[X + 3 + k] - r1[X + k]) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16)
                   | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

void pattern_132(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float sd = (float)(seed & 511) * (1.0f / 512.0f);
    float a1, a2, a3, c1, s1, c2, s2, c3, s3, sc, hd;
    int i;
    (void)sl;

    if (!p132_ready) p132_tabs();
    p132_build_hue(pal);
    memset(p132_acc, 0, sizeof p132_acc);

    /* two simultaneous 4D plane rotations + a slow xz tilt */
    a1 = t * 0.00252f + sd * 6.2831f;
    a2 = t * 0.00163f + sd * 2.7f;
    a3 = t * 0.00071f;
    c1 = cosf(a1); s1 = sinf(a1);
    c2 = cosf(a2); s2 = sinf(a2);
    c3 = cosf(a3); s3 = sinf(a3);
    sc = (float)AH * (0.415f + 0.038f * sinf(t * 0.00061f));
    hd = t * 0.00017f + sd;

    for (i = 0; i < NV; i++) {
        float x = p132_v[i][0], y = p132_v[i][1];
        float z = p132_v[i][2], u = p132_v[i][3];
        float nx, ny, nz, nu, k4, k3;
        /* xy rotation */
        nx = x * c1 - y * s1; ny = x * s1 + y * c1; x = nx; y = ny;
        /* zw rotation (the isoclinic partner) */
        nz = z * c2 - u * s2; nu = z * s2 + u * c2; z = nz; u = nu;
        /* slow xz tilt so the solid never sits flat */
        nx = x * c3 - z * s3; nz = x * s3 + z * c3; x = nx; z = nz;
        /* the second copy counter-rotates in yw */
        if (i >= 16) {
            ny = y * c2 + u * s2; nu = -y * s2 + u * c2; y = ny; u = nu;
        }
        k4 = 2.55f / (2.55f - u);
        x *= k4; y *= k4; z *= k4;
        k3 = 3.05f / (3.05f - z);
        p132_px[i] = (float)AW * 0.5f + x * k3 * sc;
        p132_py[i] = (float)AH * 0.5f + y * k3 * sc;
        p132_pw[i] = u;
        p132_pz[i] = k3;
    }

    for (i = 0; i < NE; i++) {
        int A = p132_e[i][0], B = p132_e[i][1];
        int ha = (int)((p132_pw[A] * 0.26f + p132_pz[A] * 0.30f + hd) * 512.0f) & 511;
        int hb = (int)((p132_pw[B] * 0.26f + p132_pz[B] * 0.30f + hd) * 512.0f) & 511;
        float ba = 0.20f + 0.55f * p132_pz[A];
        float bb = 0.20f + 0.55f * p132_pz[B];
        if (A >= 16) { ba *= 0.62f; bb *= 0.62f; }
        p132_line(p132_px[A], p132_py[A], p132_px[B], p132_py[B],
                  p132_hue[ha], p132_hue[hb], ba, bb);
    }
    /* vertex sparks */
    for (i = 0; i < NV; i++) {
        int hi = (int)((p132_pw[i] * 0.26f + p132_pz[i] * 0.30f + hd) * 512.0f) & 511;
        float c[3], b = (0.9f + 1.5f * p132_pz[i]) * (i >= 16 ? 0.6f : 1.0f);
        c[0] = p132_hue[hi][0] * b; c[1] = p132_hue[hi][1] * b;
        c[2] = p132_hue[hi][2] * b;
        p132_splat(p132_px[i], p132_py[i], c, 1.0f);
    }

    p132_bloom();
    p132_resolve();
    p132_upscale(fb, w, h);
}
