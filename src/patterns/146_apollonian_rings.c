/* 146 Apollonian Rings — a circle packing, drawn as rings and nothing else.
 * The gasket is generated once from the complex Descartes theorem: a circle is
 * the vector (b, b*x, b*y) with b = 1/r, and the "other" circle tangent to a
 * triple is simply  s' = 2(s1+s2+s3) - s4  — a linear reflection, so the whole
 * packing falls out of one recursion with no square roots. ~600 circles down to
 * r = 1/170 of the outer disc. Nothing is filled: each circle is stroked as an
 * antialiased 2.1px ring in a soft halo, whose colour comes from log(radius), so the packing
 * reads as nested jewellery on black. The gasket turns about a third of a
 * degree a second and breathes 4%, and every ring has its own slow radial
 * shimmer. Almost all black — the ideal top layer. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stddef.h>

#define P146_MAX 640
#define P146_MAXBEND 172.0f

static float p146_x[P146_MAX], p146_y[P146_MAX], p146_r[P146_MAX];
static uint16_t p146_ph[P146_MAX];
static int p146_n, p146_ready;
static float p146_sin[1024];

static inline float p146_s(int i) { return p146_sin[i & 1023]; }

static void p146_emit(float b, float bx, float by)
{
    if (p146_n >= P146_MAX) return;
    float r = 1.0f / b;
    p146_x[p146_n] = bx / b;
    p146_y[p146_n] = by / b;
    p146_r[p146_n] = r;
    p146_ph[p146_n] = (uint16_t)((p146_n * 397u) & 1023u);
    p146_n++;
}

/* quadruple (s1,s2,s3,s4) with s4 newest; replace each of the first three */
static void p146_rec(const float *s1, const float *s2, const float *s3,
                     const float *s4, int depth)
{
    if (depth > 14 || p146_n >= P146_MAX) return;
    const float *tri[3][3] = { { s2, s3, s4 }, { s1, s3, s4 }, { s1, s2, s4 } };
    const float *old[3] = { s1, s2, s3 };
    for (int k = 0; k < 3; k++) {
        float nb[3];
        for (int c = 0; c < 3; c++)
            nb[c] = 2.0f * (tri[k][0][c] + tri[k][1][c] + tri[k][2][c]) - old[k][c];
        if (nb[0] > P146_MAXBEND || nb[0] <= 0.0f) continue;
        p146_emit(nb[0], nb[1], nb[2]);
        p146_rec(tri[k][0], tri[k][1], tri[k][2], nb, depth + 1);
    }
}

static void p146_init(void)
{
    for (int i = 0; i < 1024; i++)
        p146_sin[i] = sinf((float)i * (6.28318531f / 1024.0f));
    /* seed quadruple: bends -1, 2, 2, 3 (and the mirror 3) */
    static float c0[3] = { -1.0f,  0.0f,        0.0f };
    static float c1[3] = {  2.0f, -1.0f,        0.0f };
    static float c2[3] = {  2.0f,  1.0f,        0.0f };
    static float c3[3] = {  3.0f,  0.0f,       -2.0f };
    static float c4[3] = {  3.0f,  0.0f,        2.0f };
    p146_n = 0;
    p146_emit(1.0f, 0.0f, 0.0f);            /* outer rim, drawn as r = 1 */
    p146_emit(c1[0], c1[1], c1[2]);
    p146_emit(c2[0], c2[1], c2[2]);
    p146_emit(c3[0], c3[1], c3[2]);
    p146_emit(c4[0], c4[1], c4[2]);
    p146_rec(c0, c1, c2, c3, 0);
    p146_rec(c0, c1, c2, c4, 0);
    p146_ready = 1;
}

void pattern_146(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (!p146_ready) p146_init();

    const float t = (float)frame;
    const int sp = (int)(seed & 1023u);

    /* dark ground: a vignette in the palette's shadow end */
    uint32_t gc = pal[(uint32_t)((int)(t * 0.7f) + 1500 + sp) & JD_PAL_MASK];
    int gr = (int)((gc >> 16) & 255u) >> 4;
    int gg = (int)((gc >> 8) & 255u) >> 4;
    int gb = (int)(gc & 255u) >> 4;
    for (int y = 0; y < h; y++) {
        int dy = y - h / 2;
        int v = 255 - (dy * dy * 220) / (h * h / 2 + 1);
        if (v < 24) v = 24;
        uint32_t c = 0xFF000000u | ((uint32_t)((gr * v) >> 9) << 16)
                   | ((uint32_t)((gg * v) >> 9) << 8) | (uint32_t)((gb * v) >> 9);
        uint32_t *row = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) row[x] = c;
    }

    const float cx = (float)w * 0.5f, cy = (float)h * 0.5f;
    const float R = 0.465f * (float)(w < h ? w : h)
                  * (1.0f + 0.040f * p146_s((int)(t * 0.42f)));
    const float rot = t * 0.00055f + (float)sp * 0.0061f;
    const float rc = cosf(rot), rs = sinf(rot);
    const float lw = 2.60f, ilw = 1.0f / lw;
    const float gw = 8.0f, igw = 1.0f / gw;   /* halo half-width */
    const int cidx = (int)(t * 1.1f) + (int)(seed & 8191u);

    for (int i = 0; i < p146_n; i++) {
        float ux = p146_x[i], uy = p146_y[i];
        float sh = 1.0f + 0.030f * p146_s((int)(t * 0.55f) + p146_ph[i]);
        float rr = p146_r[i] * R * sh;
        if (rr < 1.0f) continue;
        float px = cx + (ux * rc - uy * rs) * R;
        float py = cy + (ux * rs + uy * rc) * R;

        /* colour: small circles run further along the palette */
        int band = (int)(logf(p146_r[i] * 12.0f + 0.02f) * -2600.0f);
        uint32_t col = pal[(uint32_t)(cidx + band) & JD_PAL_MASK];
        int cr8 = (int)((col >> 16) & 255u), cg8 = (int)((col >> 8) & 255u);
        int cb8 = (int)(col & 255u);
        float glow = 0.68f + 0.32f * p146_s((int)(t * 0.8f) + p146_ph[i] * 2);

        float ro = rr + gw, ri = rr - gw;
        int y0 = (int)(py - ro), y1 = (int)(py + ro) + 1;
        if (y0 < 0) y0 = 0;
        if (y1 > h) y1 = h;
        float ro2 = ro * ro, ri2 = ri > 0.0f ? ri * ri : 0.0f;
        for (int y = y0; y < y1; y++) {
            float dy = (float)y + 0.5f - py;
            float dy2 = dy * dy;
            if (dy2 >= ro2) continue;
            float xo = sqrtf(ro2 - dy2);
            float xi = (ri2 > dy2) ? sqrtf(ri2 - dy2) : -1.0f;
            uint32_t *row = fb + (size_t)y * (size_t)w;
            for (int pass = 0; pass < 2; pass++) {
                int xa, xb;
                if (xi < 0.0f) {
                    if (pass) break;
                    xa = (int)(px - xo); xb = (int)(px + xo) + 1;
                } else if (pass == 0) {
                    xa = (int)(px - xo); xb = (int)(px - xi) + 1;
                } else {
                    xa = (int)(px + xi); xb = (int)(px + xo) + 1;
                }
                if (xa < 0) xa = 0;
                if (xb > w) xb = w;
                for (int x = xa; x < xb; x++) {
                    float dx = (float)x + 0.5f - px;
                    float d = sqrtf(dx * dx + dy2) - rr;
                    if (d < 0.0f) d = -d;
                    float cov = 1.0f - d * ilw;
                    float hal = 1.0f - d * igw;
                    if (hal <= 0.0f) continue;
                    float amt = hal * hal * 0.38f;
                    if (cov > 0.0f) amt += cov * cov * 2.20f;
                    amt *= glow;
                    int v8 = (int)(amt * 255.0f);
                    if (v8 > 255) v8 = 255;
                    uint32_t o = row[x];
                    int r = (int)((o >> 16) & 255u) + ((cr8 * v8) >> 8);
                    int g = (int)((o >> 8) & 255u) + ((cg8 * v8) >> 8);
                    int b = (int)(o & 255u) + ((cb8 * v8) >> 8);
                    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
                    row[x] = 0xFF000000u | ((uint32_t)r << 16)
                           | ((uint32_t)g << 8) | (uint32_t)b;
                }
            }
        }
    }
}
