/* 129 Torus Knot — a glowing (p,q) knot tumbling in three dimensions.
 * The curve is the standard torus knot
 *     x = (R + a cos qt) cos pt,  y = (R + a cos qt) sin pt,  z = a sin qt
 * traced with 3000 samples, rotated by two slowly precessing Euler angles and
 * projected with a real perspective divide. Each sample is stamped as a soft
 * bead whose radius and brightness follow the depth term, so the strand reads
 * as a tube passing in front of and behind itself: the near pass is fat and hot,
 * the far pass thin and dim, and the crossings are unambiguous.
 * (p,q) is drawn from the segment seed — 2:3, 3:5, 2:7 and friends are all
 * genuinely different knots — while R, a and the tumble keep morphing inside
 * the segment. Blending is purely additive, so no depth sort is needed.
 * Overlay routine: one bright strand, everything else black. */
#include "../jellydazzle.h"
#include <math.h>

#define P129_LW  640
#define P129_LH  480
#define P129_N   (P129_LW * P129_LH)
#define P129_SMP 3000
#define P129_TAU 6.283185307179586f

static uint32_t p129_low[P129_N];
static uint32_t p129_ramp[256];

static void p129_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P129_LW << 16) / w);
    int fx0 = (int)(((long)P129_LW << 15) / w) - (1 << 15);
    int maxx = (P129_LW - 1) << 16, maxy = (P129_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P129_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * (long)w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P129_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p129_low + (long)y0 * P129_LW;
        r1 = p129_low + (long)y1 * P129_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P129_LW ? x0 + 1 : x0;
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

static void p129_splat(float fx, float fy, int r, int g, int b, int wq)
{
    int xi = (int)fx, yi = (int)fy, k;
    unsigned wx, wy2;
    if (xi < 1 || yi < 1 || xi >= P129_LW - 2 || yi >= P129_LH - 2) return;
    wx  = (unsigned)((fx - (float)xi) * 256.0f);
    wy2 = (unsigned)((fy - (float)yi) * 256.0f);
    for (k = 0; k < 4; k++) {
        unsigned kw = (k & 1 ? wx : 256u - wx) * (k & 2 ? wy2 : 256u - wy2);
        int q = (int)(((kw >> 8) * (unsigned)wq) >> 8);
        uint32_t *p = &p129_low[(yi + ((k >> 1) & 1)) * P129_LW + xi + (k & 1)];
        uint32_t v = *p;
        int rr = (int)((v >> 16) & 255) + ((r * q) >> 8);
        int gg = (int)((v >> 8) & 255) + ((g * q) >> 8);
        int bb = (int)(v & 255) + ((b * q) >> 8);
        if (rr > 255) rr = 255; if (gg > 255) gg = 255; if (bb > 255) bb = 255;
        *p = 0xFF000000u | ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb;
    }
}

/* a bead of screen radius rad, drawn as a centre plus a ring of four */
static void p129_bead(float fx, float fy, float rad, int r, int g, int b, int wq)
{
    int h = (int)((float)wq * 0.42f);
    p129_splat(fx, fy, r, g, b, wq);
    if (rad < 0.7f || h < 3) return;
    p129_splat(fx - rad, fy, r, g, b, h);
    p129_splat(fx + rad, fy, r, g, b, h);
    p129_splat(fx, fy - rad, r, g, b, h);
    p129_splat(fx, fy + rad, r, g, b, h);
    if (rad > 1.9f) {
        int h2 = (int)((float)wq * 0.20f);
        float d = rad * 0.72f;
        p129_splat(fx - d, fy - d, r, g, b, h2);
        p129_splat(fx + d, fy - d, r, g, b, h2);
        p129_splat(fx - d, fy + d, r, g, b, h2);
        p129_splat(fx + d, fy + d, r, g, b, h2);
    }
}

void pattern_129(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    static const int pq[8][2] = { {2,3},{3,2},{2,5},{5,2},{3,4},{4,3},{3,5},{2,7} };
    float t = (float)(frame & 0xFFFFF);
    float R, a, ax, ay, ca, sa, cb, sb, foc, scale;
    float cx = P129_LW * 0.5f, cy = P129_LH * 0.5f;
    int i, pb, pp, qq;
    (void)sl;

    pp = pq[seed & 7][0]; qq = pq[seed & 7][1];

    for (i = 0; i < P129_N; i++) p129_low[i] = 0xFF000000u;
    pb = (int)(t * 3.5f);
    for (i = 0; i < 256; i++)
        p129_ramp[i] = pal[(pb + i * 128) & JD_PAL_MASK] & 0x00FFFFFFu;

    R  = 1.00f;
    a  = 0.36f + 0.11f * sinf(t * 0.00047f);
    ax = t * 0.00061f;
    ay = t * 0.00043f + 0.9f;
    ca = cosf(ax); sa = sinf(ax);
    cb = cosf(ay); sb = sinf(ay);
    foc   = 3.4f;
    scale = (float)P129_LH * 0.232f;

    for (i = 0; i < P129_SMP; i++) {
        float u  = (float)i * (P129_TAU / (float)P129_SMP);
        float cq = cosf((float)qq * u), sq = sinf((float)qq * u);
        float rr = R + a * cq;
        float x0 = rr * cosf((float)pp * u);
        float y0 = rr * sinf((float)pp * u);
        float z0 = a * sq;
        float x1, y1, z1, x2, y2, z2, dz, ps, sxp, syp, sh, rad;
        uint32_t col;
        int cr, cg, cb2, wq, mxc;
        /* rotate about x, then about y */
        y1 = y0 * ca - z0 * sa; z1 = y0 * sa + z0 * ca; x1 = x0;
        x2 = x1 * cb + z1 * sb; z2 = -x1 * sb + z1 * cb; y2 = y1;
        dz = foc - z2;
        if (dz < 0.5f) continue;
        ps  = foc / dz;
        sxp = cx + x2 * ps * scale;
        syp = cy + y2 * ps * scale;
        sh  = ps * ps;                      /* near strands hotter and fatter */
        rad = 0.55f + 1.75f * (sh - 0.55f);
        if (rad < 0.4f) rad = 0.4f; if (rad > 3.4f) rad = 3.4f;
        col = p129_ramp[((i * 256) / P129_SMP + (int)(t * 0.65f)) & 255];
        cr = (int)((col >> 16) & 255); cg = (int)((col >> 8) & 255);
        cb2 = (int)(col & 255);
        mxc = cr > cg ? cr : cg; if (cb2 > mxc) mxc = cb2;
        if (mxc < 80) { cr += 80 - mxc; cg += 80 - mxc; cb2 += 80 - mxc; }
        wq = (int)(300.0f * sh);
        if (wq < 4) continue;
        if (wq > 430) wq = 430;
        p129_bead(sxp, syp, rad, cr, cg, cb2, wq);
    }
    p129_blit(fb, w, h);
}
