/* 147 Tesseract Drift — three nested hypercubes turning in four dimensions.
 * The 16 vertices of the 4-cube are rotated in the xw, yz, xy and zw planes at
 * four incommensurate rates, perspective-projected 4D->3D and then 3D->2D, and
 * the 32 edges are stroked as additive glow. Because the 4-space rotation is
 * genuine, edges swing *through* each other and the solid appears to turn
 * inside out — the classic tesseract illusion, not a spinning wireframe box.
 * Palette index follows the interpolated w coordinate, so an edge changes hue
 * along its length as it passes through the fourth dimension, and brightness
 * follows the perspective factor so near edges bloom and far ones recede.
 * Three copies at 1.00 / 0.46 / 0.20 scale counter-rotate inside each other.
 * Pure line art on black. */
#include "../jellydazzle.h"
#include <math.h>
#include <stddef.h>

static const int8_t p147_e[32][2] = {
    {0,1},{0,2},{0,4},{0,8},{1,3},{1,5},{1,9},{2,3},{2,6},{2,10},{3,7},{3,11},
    {4,5},{4,6},{4,12},{5,7},{5,13},{6,7},{6,14},{7,15},{8,9},{8,10},{8,12},
    {9,11},{9,13},{10,11},{10,14},{11,15},{12,13},{12,14},{13,15},{14,15}
};

static inline void p147_splat(uint32_t *fb, int w, int h, float fx, float fy,
                              int r, int g, int b, float amt)
{
    int xi = (int)fx, yi = (int)fy;
    if ((unsigned)xi >= (unsigned)(w - 1) || (unsigned)yi >= (unsigned)(h - 1)) return;
    float u = fx - (float)xi, v = fy - (float)yi;
    float wt[4];
    wt[0] = (1.0f - u) * (1.0f - v) * amt;
    wt[1] = u * (1.0f - v) * amt;
    wt[2] = (1.0f - u) * v * amt;
    wt[3] = u * v * amt;
    uint32_t *p = fb + (size_t)yi * (size_t)w + xi;
    int off[4]; off[0] = 0; off[1] = 1; off[2] = w; off[3] = w + 1;
    for (int k = 0; k < 4; k++) {
        int a8 = (int)(wt[k] * 255.0f);
        if (a8 <= 0) continue;
        if (a8 > 255) a8 = 255;
        uint32_t o = p[off[k]];
        int rr = (int)((o >> 16) & 255u) + ((r * a8) >> 8);
        int gg = (int)((o >> 8) & 255u) + ((g * a8) >> 8);
        int bb = (int)(o & 255u) + ((b * a8) >> 8);
        if (rr > 255) rr = 255; if (gg > 255) gg = 255; if (bb > 255) bb = 255;
        p[off[k]] = 0xFF000000u | ((uint32_t)rr << 16)
                  | ((uint32_t)gg << 8) | (uint32_t)bb;
    }
}

static void p147_obj(uint32_t *fb, int w, int h, const float ang[4],
                     float S, float bright, int cbase, const uint32_t *pal)
{
    float c0 = cosf(ang[0]), s0 = sinf(ang[0]);
    float c1 = cosf(ang[1]), s1 = sinf(ang[1]);
    float c2 = cosf(ang[2]), s2 = sinf(ang[2]);
    float c3 = cosf(ang[3]), s3 = sinf(ang[3]);
    float px[16], py[16], pw[16], pk[16];
    const float cx = (float)w * 0.5f, cy = (float)h * 0.5f;

    for (int i = 0; i < 16; i++) {
        float x = (i & 1) ? 1.0f : -1.0f;
        float y = (i & 2) ? 1.0f : -1.0f;
        float z = (i & 4) ? 1.0f : -1.0f;
        float ww = (i & 8) ? 1.0f : -1.0f;
        float t1;
        t1 = x * c0 - ww * s0; ww = x * s0 + ww * c0; x = t1;   /* xw plane */
        t1 = y * c1 - z * s1;  z  = y * s1 + z * c1;  y = t1;   /* yz plane */
        t1 = x * c2 - y * s2;  y  = x * s2 + y * c2;  x = t1;   /* xy plane */
        t1 = z * c3 - ww * s3; ww = z * s3 + ww * c3; z = t1;   /* zw plane */
        float k4 = 1.0f / (2.85f - ww);
        x *= k4; y *= k4; z *= k4;
        float k3 = 1.0f / (1.55f - z);
        px[i] = cx + x * k3 * S;
        py[i] = cy + y * k3 * S;
        pw[i] = ww;
        pk[i] = k3 * k4 * 2.4f;
    }

    for (int e = 0; e < 32; e++) {
        int a = p147_e[e][0], b = p147_e[e][1];
        float x0 = px[a], y0 = py[a], x1 = px[b], y1 = py[b];
        float dx = x1 - x0, dy = y1 - y0;
        float len = sqrtf(dx * dx + dy * dy);
        int n = (int)(len * 2.0f) + 2;
        if (n > 2400) n = 2400;
        float inv = 1.0f / (float)(n - 1);
        float amt = bright * (0.55f / (float)n) * len;
        if (amt > 1.20f) amt = 1.20f;
        for (int s = 0; s < n; s++) {
            float u = (float)s * inv;
            float ww = pw[a] + (pw[b] - pw[a]) * u;
            float kk = pk[a] + (pk[b] - pk[a]) * u;
            uint32_t col = pal[(uint32_t)(cbase + (int)(ww * 2600.0f)) & JD_PAL_MASK];
            int r = (int)((col >> 16) & 255u), g = (int)((col >> 8) & 255u);
            int bl = (int)(col & 255u);
            float aa = amt * kk;
            float fx = x0 + dx * u, fy = y0 + dy * u;
            p147_splat(fb, w, h, fx, fy, r, g, bl, aa);
            p147_splat(fb, w, h, fx + 1.3f, fy, r, g, bl, aa * 0.30f);
            p147_splat(fb, w, h, fx - 1.3f, fy, r, g, bl, aa * 0.30f);
            p147_splat(fb, w, h, fx, fy + 1.3f, r, g, bl, aa * 0.30f);
            p147_splat(fb, w, h, fx, fy - 1.3f, r, g, bl, aa * 0.30f);
        }
    }
    /* vertices as small blooms */
    for (int i = 0; i < 16; i++) {
        uint32_t col = pal[(uint32_t)(cbase + (int)(pw[i] * 2600.0f) + 700) & JD_PAL_MASK];
        int r = (int)((col >> 16) & 255u), g = (int)((col >> 8) & 255u);
        int bl = (int)(col & 255u);
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++) {
                float d = (float)(dx * dx + dy * dy);
                float a = bright * pk[i] * 0.55f / (1.0f + d * 1.4f);
                p147_splat(fb, w, h, px[i] + (float)dx, py[i] + (float)dy, r, g, bl, a);
            }
    }
}

void pattern_147(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    const float t = (float)frame;
    const float sd = (float)(seed & 1023u) * 0.006136f;

    /* ground: near-black wash from the palette's shadow end */
    uint32_t gc = pal[(uint32_t)((int)(t * 0.6f) + 1200 + (int)(seed & 4095u)) & JD_PAL_MASK];
    int gr = (int)((gc >> 16) & 255u) >> 5, gg = (int)((gc >> 8) & 255u) >> 5;
    int gb = (int)(gc & 255u) >> 5;
    for (int y = 0; y < h; y++) {
        int dy = y - h / 2;
        int v = 255 - (dy * dy * 200) / (h * h / 2 + 1);
        if (v < 30) v = 30;
        uint32_t c = 0xFF000000u | ((uint32_t)((gr * v) >> 8) << 16)
                   | ((uint32_t)((gg * v) >> 8) << 8) | (uint32_t)((gb * v) >> 8);
        uint32_t *row = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) row[x] = c;
    }

    const float S = 0.46f * (float)(w < h ? w : h);
    const int cidx = (int)(t * 1.2f) + (int)(seed & 8191u);
    float a[4];

    a[0] = t * 0.00291f + sd;
    a[1] = t * 0.00203f + sd * 1.7f;
    a[2] = t * 0.00119f + sd * 0.6f;
    a[3] = t * 0.00167f + sd * 2.3f;
    p147_obj(fb, w, h, a, S, 2.30f, cidx, pal);

    a[0] = -t * 0.00233f + sd * 2.9f;
    a[1] =  t * 0.00311f + sd * 0.4f;
    a[2] = -t * 0.00147f + sd * 1.1f;
    a[3] =  t * 0.00097f + sd * 3.1f;
    p147_obj(fb, w, h, a, S * 0.46f, 1.90f, cidx + 5200, pal);

    a[0] =  t * 0.00389f + sd * 1.3f;
    a[1] = -t * 0.00251f + sd * 2.2f;
    a[2] =  t * 0.00181f + sd * 0.9f;
    a[3] = -t * 0.00133f + sd * 1.8f;
    p147_obj(fb, w, h, a, S * 0.20f, 1.45f, cidx + 10400, pal);
}
