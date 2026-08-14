/* 124 Poincare Web — the {4,q} hyperbolic tiling, drawn as its mirror lines.
 * The (2,4,q) triangle group has two straight mirrors 45 degrees apart and one
 * geodesic mirror: the circle orthogonal to the unit circle with
 *     d = cos(pi/q) / sqrt(cos^2(pi/4) - sin^2(pi/q)),   rc^2 = d^2 - 1.
 * Because the straight pair is exactly the octant fold, folding a point into
 * the fundamental triangle costs an abs, an abs, a compare-and-swap and one
 * circle inversion per round — no atan2 anywhere. Each inversion multiplies the
 * local derivative by rc^2/|z-c|^2, and carrying that product lets the mirror
 * distance be converted back to SCREEN units, so the lines keep a constant
 * pixel width all the way to the rim instead of turning into a bright ring.
 * A slow Mobius translation (z+a)/(1+conj(a)z) glides the whole hyperbolic
 * plane past the window, which is the one motion Euclidean tilings cannot do.
 * Overlay routine: line art on black inside a disk, black outside it. */
#include "../jellydazzle.h"
#include <math.h>

#define P124_LW  640
#define P124_LH  480
#define P124_N   (P124_LW * P124_LH)
#define P124_IT  21

static uint32_t p124_low[P124_N];
static uint8_t  p124_lut[512];          /* px distance*32 -> intensity      */
static uint32_t p124_ramp[256];
static int      p124_ready;

static void p124_init(void)
{
    int i;
    for (i = 0; i < 512; i++) {
        float a = (float)i * (1.0f / 32.0f);
        float v = 1.9f / (1.9f + a * a);
        p124_lut[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    p124_ready = 1;
}

static void p124_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P124_LW << 16) / w);
    int fx0 = (int)(((long)P124_LW << 15) / w) - (1 << 15);
    int maxx = (P124_LW - 1) << 16, maxy = (P124_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P124_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * (long)w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P124_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p124_low + (long)y0 * P124_LW;
        r1 = p124_low + (long)y1 * P124_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P124_LW ? x0 + 1 : x0;
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

void pattern_124(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float qn, cb2, d, rc2, rc, R, invR, ar, ai, aa, rot, cr_, sr_;
    int x, y, i, pb;
    (void)sl;

    if (!p124_ready) p124_init();

    /* q glides 5 -> 7 -> 5: the vertex figure itself breathes */
    qn  = 6.0f + 1.35f * sinf(t * 0.00019f + (float)(seed & 7) * 0.8f);
    cb2 = cosf(3.14159265f / qn); cb2 = cb2 * cb2;
    d   = sqrtf(cb2 / (0.5f - (1.0f - cb2)));
    rc2 = d * d - 1.0f;
    rc  = sqrtf(rc2);
    R   = 232.0f;
    invR = 1.0f / R;

    /* hyperbolic translation: |a| stays well inside the disk */
    aa  = 0.42f + 0.24f * sinf(t * 0.00027f);
    ar  = aa * cosf(t * 0.00041f);
    ai  = aa * sinf(t * 0.00041f);
    rot = t * 0.00023f;
    cr_ = cosf(rot); sr_ = sinf(rot);

    pb = (int)(t * 4.0f);
    for (i = 0; i < 256; i++)
        p124_ramp[i] = pal[(pb + i * 128) & JD_PAL_MASK] & 0x00FFFFFFu;

    for (y = 0; y < P124_LH; y++) {
        float py = ((float)y - P124_LH * 0.5f) * invR;
        uint32_t *dst = p124_low + (long)y * P124_LW;
        for (x = 0; x < P124_LW; x++) {
            float px = ((float)x - P124_LW * 0.5f) * invR;
            float zx, zy, ux, uy, den, sc, rr, dx, q2, dl, e1, e2;
            float fade;
            int it, iv, hi, cr, cg, cb, c;
            uint32_t col;
            rr = px * px + py * py;
            /* the rim fade reaches zero here, so skip the iteration entirely */
            fade = (0.945f - rr) * 4.2f;
            if (fade <= 0.0f) { dst[x] = 0xFF000000u; continue; }
            if (fade > 1.0f) fade = 1.0f;
            /* rotate, then Mobius-translate by a */
            zx = px * cr_ - py * sr_;
            zy = px * sr_ + py * cr_;
            ux = zx + ar; uy = zy + ai;
            /* denominator 1 + conj(a) z = (1 + ar*zx + ai*zy) + i(ar*zy - ai*zx) */
            {
                float dr = 1.0f + ar * zx + ai * zy;
                float di = ar * zy - ai * zx;
                den = dr * dr + di * di;
                if (den < 1e-6f) den = 1e-6f;
                zx = (ux * dr + uy * di) / den;
                zy = (uy * dr - ux * di) / den;
            }
            sc = (1.0f - aa * aa) / den;      /* |dz'/dz| of the Mobius map  */
            rr = zx * zx + zy * zy;
            if (rr > 0.99992f) { dst[x] = 0xFF000000u; continue; }

            for (it = 0; it < P124_IT; it++) {
                float k;
                zx = zx < 0.0f ? -zx : zx;
                zy = zy < 0.0f ? -zy : zy;
                if (zy > zx) { float s = zx; zx = zy; zy = s; }
                dx = zx - d; q2 = dx * dx + zy * zy;
                if (q2 >= rc2) break;
                if (q2 < 1e-9f) q2 = 1e-9f;
                k = rc2 / q2;
                zx = d + dx * k; zy = zy * k; sc *= k;
            }
            if (it >= P124_IT) { dst[x] = 0xFF000000u; continue; }

            dl = sqrtf(q2) - rc;                    /* to the geodesic mirror */
            if (dl < 0.0f) dl = -dl;
            e1 = zy;                                /* to the x-axis mirror   */
            e2 = (zx - zy) * 0.70710678f;           /* to the 45-degree mirror*/
            {
                float s = R / sc;
                int i1 = (int)(e1 * s * 32.0f), i2 = (int)(e2 * s * 32.0f);
                int i3 = (int)(dl * s * 32.0f);
                iv  = i1 < 511 ? (int)p124_lut[i1 < 0 ? 0 : i1] : 0;
                iv += i2 < 511 ? (int)p124_lut[i2 < 0 ? 0 : i2] : 0;
                iv += i3 < 511 ? (int)p124_lut[i3 < 0 ? 0 : i3] : 0;
            }
            if (iv < 3) { dst[x] = 0xFF000000u; continue; }
            iv = (int)((float)iv * fade);
            if (iv > 255) { c = iv - 255; iv = 255; if (c > 180) c = 180; }
            else c = 0;
            hi  = (it * 27 + (int)(t * 0.7f)) & 255;
            col = p124_ramp[hi];
            cr = (int)((((col >> 16) & 255) * iv) >> 8) + c;
            cg = (int)((((col >> 8) & 255) * iv) >> 8) + c;
            cb = (int)(((col & 255) * iv) >> 8) + c;
            if (cr > 255) cr = 255; if (cg > 255) cg = 255; if (cb > 255) cb = 255;
            dst[x] = 0xFF000000u | ((uint32_t)cr << 16) |
                     ((uint32_t)cg << 8) | (uint32_t)cb;
        }
    }
    p124_blit(fb, w, h);
}
