/* 123 Apollonian — the Apollonian gasket, drawn as light rings on black.
 * Three equal circles are packed in a unit disk and then Descartes' circle
 * theorem is run backwards forever: any three mutually tangent circles admit a
 * fourth, and the "other" solution of the quadruple is simply
 *     b' = 2(b1+b2+b3) - b4      (b'z') = 2(b1z1+b2z2+b3z3) - b4z4
 * with b the curvature and z the centre as a complex number. Recursing on that
 * reflection to a curvature ceiling gives a few hundred circles whose sizes
 * fall off geometrically — the classic self-similar packing.
 * The set is built once at init; each frame only re-stamps the outlines, so
 * the cost is the sum of the circumferences (~25k soft splats), not a per-pixel
 * field. The gasket turns about a fifth of a degree a second and each depth
 * breathes on its own phase, with hue taken from the palette by depth.
 * Overlay routine: pure line art, everything between the rings is black. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P123_LW   640
#define P123_LH   480
#define P123_N    (P123_LW * P123_LH)
#define P123_MAXC 900
#define P123_TAU  6.283185307179586f
#define P123_BMAX 155.0f
#define P123_R    218.0f

typedef struct { float b, x, y; int d; } p123_circ;

static p123_circ p123_c[P123_MAXC];
static int       p123_nc;
static uint32_t  p123_low[P123_N];
static int       p123_ready;

static void p123_add(float b, float x, float y, int d)
{
    if (p123_nc >= P123_MAXC) return;
    p123_c[p123_nc].b = b; p123_c[p123_nc].x = x;
    p123_c[p123_nc].y = y; p123_c[p123_nc].d = d;
    p123_nc++;
}

/* Breadth-first Descartes reflection: each quadruple spawns three more, and
 * BFS order means the circles are generated largest-first, so a fixed budget
 * always spends itself on the visible ones. */
typedef struct { float b[4], x[4], y[4]; int skip, d; } p123_quad;

static p123_quad p123_q[P123_MAXC + 8];

static void p123_init(void)
{
    float r0 = 2.0f * 1.7320508f - 3.0f, dd, bb;
    int i, head = 0, tail = 0;
    p123_quad *q0 = &p123_q[0];
    p123_nc = 0;
    dd = 1.0f - r0;
    bb = 1.0f / r0;
    q0->b[0] = -1.0f; q0->x[0] = 0.0f; q0->y[0] = 0.0f;
    p123_add(-1.0f, 0.0f, 0.0f, 0);                    /* the enclosing rim */
    for (i = 0; i < 3; i++) {
        float a = 1.5707963f + (float)i * (P123_TAU / 3.0f);
        q0->b[i + 1] = bb;
        q0->x[i + 1] = dd * cosf(a);
        q0->y[i + 1] = dd * sinf(a);
        p123_add(bb, q0->x[i + 1], q0->y[i + 1], 1);
    }
    q0->skip = -1; q0->d = 2;
    tail = 1;
    while (head < tail && p123_nc < P123_MAXC) {
        p123_quad qq = p123_q[head++];
        for (i = 0; i < 4; i++) {
            float sb = 0.0f, sx = 0.0f, sy = 0.0f, nb, nx, ny;
            int j;
            if (i == qq.skip || p123_nc >= P123_MAXC) continue;
            for (j = 0; j < 4; j++) {
                if (j == i) continue;
                sb += qq.b[j]; sx += qq.b[j] * qq.x[j]; sy += qq.b[j] * qq.y[j];
            }
            nb = 2.0f * sb - qq.b[i];
            if (!(nb > 0.0f) || nb > P123_BMAX) continue;
            nx = (2.0f * sx - qq.b[i] * qq.x[i]) / nb;
            ny = (2.0f * sy - qq.b[i] * qq.y[i]) / nb;
            p123_add(nb, nx, ny, qq.d);
            if (tail < P123_MAXC + 8) {
                p123_quad *np = &p123_q[tail++];
                *np = qq;
                np->b[i] = nb; np->x[i] = nx; np->y[i] = ny;
                np->skip = i; np->d = qq.d + 1;
            }
        }
    }
    p123_ready = 1;
}

static void p123_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P123_LW << 16) / w);
    int fx0 = (int)(((long)P123_LW << 15) / w) - (1 << 15);
    int maxx = (P123_LW - 1) << 16, maxy = (P123_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P123_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * (long)w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P123_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p123_low + (long)y0 * P123_LW;
        r1 = p123_low + (long)y1 * P123_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P123_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((cx >> 8) & 255), sx = 256u - wx;
            unsigned sy = 256u - (unsigned)wy;
            uint32_t a = r0[x0], b = r0[x1], c = r1[x0], d = r1[x1];
            uint32_t trb = (((a & 0xFF00FFu) * sx + (b & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t tg  = (((a & 0x00FF00u) * sx + (b & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t brb = (((c & 0xFF00FFu) * sx + (d & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t bg  = (((c & 0x00FF00u) * sx + (d & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t orb = ((trb * sy + brb * (unsigned)wy) >> 8) & 0xFF00FFu;
            uint32_t og  = ((tg  * sy + bg  * (unsigned)wy) >> 8) & 0x00FF00u;
            dst[x] = 0xFF000000u | orb | og;
            fx += stepx;
        }
    }
}

static void p123_splat(float fx, float fy, int r, int g, int b, int wq)
{
    int xi = (int)fx, yi = (int)fy, k;
    unsigned wx, wy2;
    if (xi < 0 || yi < 0 || xi >= P123_LW - 1 || yi >= P123_LH - 1) return;
    wx  = (unsigned)((fx - (float)xi) * 256.0f);
    wy2 = (unsigned)((fy - (float)yi) * 256.0f);
    for (k = 0; k < 4; k++) {
        unsigned kw = (k & 1 ? wx : 256u - wx) * (k & 2 ? wy2 : 256u - wy2);
        int q = (int)(((kw >> 8) * (unsigned)wq) >> 8);
        uint32_t *p = &p123_low[(yi + ((k >> 1) & 1)) * P123_LW + xi + (k & 1)];
        uint32_t v = *p;
        int rr = (int)((v >> 16) & 255) + ((r * q) >> 8);
        int gg = (int)((v >> 8) & 255) + ((g * q) >> 8);
        int bb = (int)(v & 255) + ((b * q) >> 8);
        if (rr > 255) rr = 255; if (gg > 255) gg = 255; if (bb > 255) bb = 255;
        *p = 0xFF000000u | ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb;
    }
}

void pattern_123(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    static const float off[3] = { -1.15f, 0.0f, 1.15f };
    static const int   ow[3]  = { 120, 256, 120 };
    float t = (float)(frame & 0xFFFFF);
    float spin, ca, sa, scale;
    int i, k, j, pb;
    (void)sl; (void)seed;

    if (!p123_ready) p123_init();

    for (i = 0; i < P123_N; i++) p123_low[i] = 0xFF000000u;

    spin  = t * 0.00052f;
    ca    = cosf(spin); sa = sinf(spin);
    scale = P123_R * (1.0f + 0.018f * sinf(t * 0.00061f));
    pb    = (int)(t * 3.0f);

    for (i = 0; i < p123_nc; i++) {
        float br = 1.0f / p123_c[i].b, rr;
        float cx, cy, puls;
        uint32_t col;
        int cr, cg, cb, ns;
        if (br < 0.0f) br = -br;
        rr = br * scale;
        if (rr < 1.5f) continue;
        cx = (p123_c[i].x * ca - p123_c[i].y * sa) * scale + P123_LW * 0.5f;
        cy = (p123_c[i].x * sa + p123_c[i].y * ca) * scale + P123_LH * 0.5f;
        if (cx + rr < 0.0f || cx - rr > P123_LW || cy + rr < 0.0f || cy - rr > P123_LH)
            continue;
        puls = 0.70f + 0.30f * sinf(t * 0.0085f - 0.75f * (float)p123_c[i].d);
        col  = pal[(pb + p123_c[i].d * 2900 + (i & 7) * 260) & JD_PAL_MASK];
        cr = (int)((col >> 16) & 255); cg = (int)((col >> 8) & 255);
        cb = (int)(col & 255);
        {   /* keep faint palette entries visible as line art */
            int mx = cr > cg ? cr : cg; if (cb > mx) mx = cb;
            if (mx < 60) { cr += 60 - mx; cg += 60 - mx; cb += 60 - mx; }
        }
        ns = (int)(rr * 5.2f);
        if (ns < 20) ns = 20; if (ns > 1600) ns = 1600;
        for (j = 0; j < 3; j++) {
            float ro = rr + off[j];
            int wq = (int)(puls * (float)ow[j] * 1.55f);
            float ph = 0.37f * (float)i;
            float dth = P123_TAU / (float)ns, cd = cosf(dth), sd = sinf(dth);
            float vx = cosf(ph) * ro, vy = sinf(ph) * ro, nx;
            if (ro < 1.0f) continue;
            for (k = 0; k < ns; k++) {
                p123_splat(cx + vx, cy + vy, cr, cg, cb, wq);
                nx = vx * cd - vy * sd; vy = vx * sd + vy * cd; vx = nx;
            }
        }
    }
    p123_blit(fb, w, h);
}
