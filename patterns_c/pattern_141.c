/* 141 Chladni Sand — sand on a singing plate.
 * The nodal set of a square-plate standing wave,
 *   f(x,y) = cos(n pi x)cos(m pi y) - cos(m pi x)cos(n pi y),
 * lit where |f| ~ 0: grains pile up on the zero contour and nowhere else.
 * n and m are driven continuously (not stepped through integers) so the
 * figure morphs through the whole family instead of cutting between modes.
 * A second, finer mode pair is laid over the first at 55% weight so the
 * lace crosses itself. Separable: two cos LUTs per axis per mode, two
 * multiplies per pixel. Repaint, near-black between the lines — designed
 * to sit on top of another layer. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P141_GT 128                     /* static sand-grain tile edge */

static float *p141_ax, *p141_bx, *p141_cx, *p141_dx;   /* per-column, mode 1/2 */
static float *p141_ay, *p141_by, *p141_cy, *p141_dy;   /* per-row,    mode 1/2 */
static int p141_w = -1, p141_h = -1;
static float p141_l1[1024], p141_l2[1024];
static uint8_t p141_grain[P141_GT * P141_GT];
static int p141_ready;

static void p141_init(void)
{
    int i;
    uint32_t s = 0x9E3779B9u;
    for (i = 0; i < 1024; i++) {
        float u = (float)i * (1.0f / 511.0f);          /* |f| in 0..2 */
        p141_l1[i] = expf(-26.0f * u);
        p141_l2[i] = expf(-34.0f * u);
    }
    for (i = 0; i < P141_GT * P141_GT; i++) {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        p141_grain[i] = (uint8_t)(150 + ((s >> 9) & 105));   /* 150..255 */
    }
    p141_ready = 1;
}

static void p141_size(int w, int h)
{
    free(p141_ax); free(p141_bx); free(p141_cx); free(p141_dx);
    free(p141_ay); free(p141_by); free(p141_cy); free(p141_dy);
    p141_ax = (float *)malloc(sizeof(float) * (size_t)w);
    p141_bx = (float *)malloc(sizeof(float) * (size_t)w);
    p141_cx = (float *)malloc(sizeof(float) * (size_t)w);
    p141_dx = (float *)malloc(sizeof(float) * (size_t)w);
    p141_ay = (float *)malloc(sizeof(float) * (size_t)h);
    p141_by = (float *)malloc(sizeof(float) * (size_t)h);
    p141_cy = (float *)malloc(sizeof(float) * (size_t)h);
    p141_dy = (float *)malloc(sizeof(float) * (size_t)h);
    p141_w = w; p141_h = h;
}

void pattern_141(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (!p141_ready) p141_init();
    if (w != p141_w || h != p141_h) p141_size(w, h);

    const float t = (float)frame;
    const float ph = (float)(seed & 1023u) * 0.006135923f;   /* segment phase */

    /* continuously driven mode numbers — the whole point of the pattern */
    float n1 = 3.4f + 2.6f * sinf(t * 0.00119f + ph);
    float m1 = 5.1f + 3.1f * sinf(t * 0.00083f + ph * 1.7f + 2.1f);
    float n2 = 6.7f + 3.3f * sinf(t * 0.00061f + ph * 0.6f + 4.0f);
    float m2 = 9.3f + 4.1f * sinf(t * 0.00047f + ph * 2.3f + 0.7f);

    const float kx1 = 3.14159265f * n1 / (float)(w - 1);
    const float kx1b = 3.14159265f * m1 / (float)(w - 1);
    const float ky1 = 3.14159265f * m1 / (float)(h - 1);
    const float ky1b = 3.14159265f * n1 / (float)(h - 1);
    const float kx2 = 3.14159265f * n2 / (float)(w - 1);
    const float kx2b = 3.14159265f * m2 / (float)(w - 1);
    const float ky2 = 3.14159265f * m2 / (float)(h - 1);
    const float ky2b = 3.14159265f * n2 / (float)(h - 1);

    for (int x = 0; x < w; x++) {
        float fx = (float)x;
        p141_ax[x] = cosf(kx1 * fx);
        p141_bx[x] = cosf(kx1b * fx);
        p141_cx[x] = cosf(kx2 * fx);
        p141_dx[x] = cosf(kx2b * fx);
    }
    for (int y = 0; y < h; y++) {
        float fy = (float)y;
        p141_ay[y] = cosf(ky1 * fy);
        p141_by[y] = cosf(ky1b * fy);
        p141_cy[y] = cosf(ky2 * fy);
        p141_dy[y] = cosf(ky2b * fy);
    }

    /* plate tint: a dark, slowly walking palette sample */
    uint32_t base = pal[(uint32_t)((int)(t * 0.9f) + 900) & JD_PAL_MASK];
    int br = (int)(((base >> 16) & 255u) * 24u >> 8);
    int bg = (int)(((base >> 8) & 255u) * 24u >> 8);
    int bb = (int)((base & 255u) * 32u >> 8);

    const int cidx = (int)(t * 1.7f) + (int)(seed & 4095u);
    const float inv = 1.0f / (float)(w + h);

    for (int y = 0; y < h; y++) {
        const float a1 = p141_ay[y], b1 = p141_by[y];
        const float a2 = p141_cy[y], b2 = p141_dy[y];
        const uint8_t *grow = p141_grain + ((y & (P141_GT - 1)) * P141_GT);
        uint32_t *row = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) {
            float f = p141_ax[x] * a1 - p141_bx[x] * b1;
            float g = p141_cx[x] * a2 - p141_dx[x] * b2;
            f = f < 0.0f ? -f : f;
            g = g < 0.0f ? -g : g;
            int i1 = (int)(f * 511.0f); if (i1 > 1023) i1 = 1023;
            int i2 = (int)(g * 511.0f); if (i2 > 1023) i2 = 1023;
            float v = p141_l1[i1] + 0.55f * p141_l2[i2];
            if (v > 1.0f) v = 1.0f;
            int gr = grow[x & (P141_GT - 1)];
            int v8 = (int)(v * (float)gr);              /* 0..255, grainy */
            uint32_t c = pal[(uint32_t)(cidx + (int)(v * 5200.0f)
                             + (int)((float)(x + y) * inv * 2100.0f)) & JD_PAL_MASK];
            int r = br + (int)((((c >> 16) & 255u) * (uint32_t)v8) >> 8);
            int gg = bg + (int)((((c >> 8) & 255u) * (uint32_t)v8) >> 8);
            int b = bb + (int)(((c & 255u) * (uint32_t)v8) >> 8);
            if (r > 255) r = 255; if (gg > 255) gg = 255; if (b > 255) b = 255;
            row[x] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)gg << 8) | (uint32_t)b;
        }
    }
}
