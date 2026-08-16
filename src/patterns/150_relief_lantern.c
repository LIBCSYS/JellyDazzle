/* 150 Relief Lantern — an embossed panel lit by three wandering lamps.
 * A carved height field (radial flutes, a twelve-fold rosette, a boss lattice
 * and a fine filigree ridge) is built once and reduced to a normal map. After
 * that nothing in the geometry moves: the whole image is three coloured point
 * lamps drifting across the relief on Lissajous paths, each contributing a
 * signed emboss term 74 + N.L times its own smooth distance falloff — signed,
 * so every ridge gets a lit face and a shadow face. Because only the lighting
 * changes, the motion is a slow rake of highlight across metal — about as far
 * from a strobe as this engine gets — while the carving stays razor sharp at
 * full resolution. The relief depth itself breathes 25%, which is free: it
 * scales the light vector rather than the map. This one is a ground layer:
 * stark, high-contrast, and mostly one hue plus lamp colour. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <stddef.h>

#define P150_NL 3

static int8_t *p150_nx, *p150_ny;
static uint8_t *p150_ao;
static int p150_w = -1, p150_h = -1;
static uint8_t p150_fall[1024];
static int p150_ready;

static void p150_tables(void)
{
    for (int i = 0; i < 1024; i++) {
        float d = (float)i * (1.0f / 1024.0f);      /* normalised r^2 */
        float e = 1.0f - d;
        p150_fall[i] = (uint8_t)(255.0f * e * e / (1.0f + d * 7.0f));
    }
    p150_ready = 1;
}

static float p150_height(float x, float y, float M)
{
    float r = sqrtf(x * x + y * y);
    float th = atan2f(y, x);
    float rn = r / M;
    float v = 0.0f;
    v += 0.40f * sinf(r * (22.0f / M) + 0.8f);                  /* deep flutes */
    v += 0.30f * cosf(12.0f * th) * expf(-rn * 1.6f);           /* rosette */
    v += 0.22f * cosf(6.0f * th + r * (10.0f / M));             /* twisted spokes */
    v += 0.18f * sinf(x * (36.0f / M)) * sinf(y * (36.0f / M)); /* boss lattice */
    v += 0.14f * sinf(30.0f * th + r * (48.0f / M));            /* filigree */
    v += 0.10f * sinf(r * (154.0f / M));                        /* fine rings */
    return v;
}

static void p150_build(int w, int h)
{
    free(p150_nx); free(p150_ny); free(p150_ao);
    size_t n = (size_t)w * (size_t)h;
    p150_nx = (int8_t *)malloc(n);
    p150_ny = (int8_t *)malloc(n);
    p150_ao = (uint8_t *)malloc(n);
    float *hf = (float *)malloc(sizeof(float) * n);
    const float M = (float)(w < h ? w : h) * 0.5f;
    const float cx = (float)w * 0.5f, cy = (float)h * 0.5f;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            hf[(size_t)y * w + x] = p150_height((float)x - cx, (float)y - cy, M);
    for (int y = 0; y < h; y++) {
        int ym = y > 0 ? y - 1 : y, yp = y < h - 1 ? y + 1 : y;
        for (int x = 0; x < w; x++) {
            int xm = x > 0 ? x - 1 : x, xp = x < w - 1 ? x + 1 : x;
            float gx = hf[(size_t)y * w + xp] - hf[(size_t)y * w + xm];
            float gy = hf[(size_t)yp * w + x] - hf[(size_t)ym * w + x];
            int nx = (int)(-gx * 2700.0f), ny = (int)(-gy * 2700.0f);
            if (nx > 127) nx = 127; if (nx < -127) nx = -127;
            if (ny > 127) ny = 127; if (ny < -127) ny = -127;
            p150_nx[(size_t)y * w + x] = (int8_t)nx;
            p150_ny[(size_t)y * w + x] = (int8_t)ny;
            float hv = hf[(size_t)y * w + x];
            int a = (int)((hv + 1.5f) * 70.0f);
            if (a < 20) a = 20; if (a > 255) a = 255;
            p150_ao[(size_t)y * w + x] = (uint8_t)a;
        }
    }
    free(hf);
    p150_w = w; p150_h = h;
}

void pattern_150(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (!p150_ready) p150_tables();
    if (w != p150_w || h != p150_h) p150_build(w, h);

    const float t = (float)frame;
    const float sd = (float)(seed & 1023u) * 0.006136f;
    const float M = (float)(w < h ? w : h);
    const float depth = 1.0f + 0.25f * sinf(t * 0.00061f + sd);

    int lx[P150_NL], ly[P150_NL], lvx[P150_NL], lvy[P150_NL];
    int lr[P150_NL], lg[P150_NL], lb[P150_NL], lrad[P150_NL];
    const int cidx = (int)(t * 1.0f) + (int)(seed & 8191u);
    for (int k = 0; k < P150_NL; k++) {
        float fk = (float)k;
        float a = t * (0.00196f + 0.00073f * fk) + fk * 2.1f + sd;
        float b = t * (0.00131f - 0.00029f * fk) + fk * 3.7f + sd * 1.3f;
        lx[k] = (int)((float)w * (0.5f + 0.42f * sinf(a)));
        ly[k] = (int)((float)h * (0.5f + 0.42f * sinf(b)));
        float ang = t * (0.0043f + 0.0011f * fk) + fk * 2.4f + sd;
        lvx[k] = (int)(cosf(ang) * 127.0f * depth);
        lvy[k] = (int)(sinf(ang) * 127.0f * depth);
        uint32_t c = pal[(uint32_t)(cidx + k * 9700 + 3000) & JD_PAL_MASK];
        int r = (int)((c >> 16) & 255u), g = (int)((c >> 8) & 255u), bl = (int)(c & 255u);
        int mx = r > g ? r : g; if (bl > mx) mx = bl;
        if (mx < 40) mx = 40;
        lr[k] = r * 255 / mx; lg[k] = g * 255 / mx; lb[k] = bl * 255 / mx;
        float rad = M * (0.62f + 0.16f * sinf(t * 0.00043f + fk * 1.9f));
        lrad[k] = (int)(rad * rad / 1024.0f);
        if (lrad[k] < 1) lrad[k] = 1;
    }
    uint32_t amb = pal[(uint32_t)(cidx + 600) & JD_PAL_MASK];
    int ar = (int)((amb >> 16) & 255u), ag = (int)((amb >> 8) & 255u);
    int ab = (int)(amb & 255u);

    for (int y = 0; y < h; y++) {
        const int8_t *nxr = p150_nx + (size_t)y * w;
        const int8_t *nyr = p150_ny + (size_t)y * w;
        const uint8_t *aor = p150_ao + (size_t)y * w;
        uint32_t *row = fb + (size_t)y * (size_t)w;
        int dy[P150_NL];
        for (int k = 0; k < P150_NL; k++) dy[k] = y - ly[k];
        for (int x = 0; x < w; x++) {
            int nx = nxr[x], ny = nyr[x], ao = aor[x];
            int r = (ar * ao) >> 9, g = (ag * ao) >> 9, b = (ab * ao) >> 9;
            for (int k = 0; k < P150_NL; k++) {
                int d = 74 + ((nx * lvx[k] + ny * lvy[k]) >> 7);  /* signed emboss */
                if (d <= 0) continue;
                if (d > 255) d = 255;
                int ddx = x - lx[k], ddy = dy[k];
                int q = (ddx * ddx + ddy * ddy) / lrad[k];
                if (q > 1023) continue;
                int f = p150_fall[q];
                int amt = (d * f) >> 7;
                r += (lr[k] * amt) >> 8;
                g += (lg[k] * amt) >> 8;
                b += (lb[k] * amt) >> 8;
            }
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            row[x] = 0xFF000000u | ((uint32_t)r << 16)
                   | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}
