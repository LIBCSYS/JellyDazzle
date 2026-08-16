/* 126 Orbit Lace — a Julia set drawn by orbit trap instead of escape time.
 * Every pixel is iterated under z <- z^2 + c, but the colour does not come from
 * how long it took to escape; it comes from how close the orbit ever passed to
 * a ring of radius rt about the origin:
 *     trap = min over n of | |z_n| - rt |
 * Escape-time colouring paints solid basins — a full-bleed field. Trapping
 * paints only the thin locus of points whose orbit grazed the ring, so the same
 * mathematics yields filigree on black: nested lace shells, one per iteration
 * count, that thread through each other.
 * c walks the circle |c| = 0.745 (just outside the main cardioid, where the
 * Julia sets are richly connected), so the lace re-knits continuously and never
 * snaps between topologies. The ring radius breathes on a second, slower sine.
 * Hue is the iteration index at which the closest pass happened, so each shell
 * takes its own place in the palette.
 * Overlay routine: sparse curves on black. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P126_LW 512
#define P126_LH 384
#define P126_N  (P126_LW * P126_LH)
#define P126_IT 19

static uint32_t p126_low[P126_N];
static uint16_t p126_lut[1024];         /* trap*512 -> intensity            */
static uint32_t p126_ramp[256];
static int      p126_ready;

static void p126_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float a = (float)i * (1.0f / 512.0f);
        float v = 1.0f / (1.0f + (a * 17.0f) * (a * 17.0f));
        p126_lut[i] = (uint16_t)(v * 1023.0f + 0.5f);
    }
    p126_ready = 1;
}

static void p126_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P126_LW << 16) / w);
    int fx0 = (int)(((long)P126_LW << 15) / w) - (1 << 15);
    int maxx = (P126_LW - 1) << 16, maxy = (P126_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P126_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * (long)w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P126_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p126_low + (long)y0 * P126_LW;
        r1 = p126_low + (long)y1 * P126_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P126_LW ? x0 + 1 : x0;
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

void pattern_126(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float cre, cim, rt, rt2, zoom, ca, sa, sp, ph;
    int x, y, i, pb;
    (void)sl;

    if (!p126_ready) p126_init();

    ph  = t * 0.00047f + (float)(seed & 15) * 0.39f;
    sp  = 0.745f + 0.016f * sinf(t * 0.00021f);
    cre = sp * cosf(ph);
    cim = sp * sinf(ph);
    rt  = 0.62f + 0.30f * sinf(t * 0.00013f + 1.4f);
    rt2 = rt * rt;

    zoom = (1.62f + 0.10f * sinf(t * 0.00025f)) / (float)(P126_LH / 2);
    ca = cosf(t * 0.00019f) * zoom;
    sa = sinf(t * 0.00019f) * zoom;

    pb = (int)(t * 3.5f);
    for (i = 0; i < 256; i++)
        p126_ramp[i] = pal[(pb + i * 128) & JD_PAL_MASK] & 0x00FFFFFFu;

    for (y = 0; y < P126_LH; y++) {
        float py = (float)y - P126_LH * 0.5f;
        uint32_t *dst = p126_low + (long)y * P126_LW;
        for (x = 0; x < P126_LW; x++) {
            float px = (float)x - P126_LW * 0.5f;
            float zr = px * ca - py * sa;
            float zi = px * sa + py * ca;
            float best = 1e9f, zr2, zi2;
            int bi = 0, k, iv, cr, cg, cb, hot;
            uint32_t col;
            for (k = 0; k < P126_IT; k++) {
                float q;
                zr2 = zr * zr; zi2 = zi * zi;
                if (zr2 + zi2 > 36.0f) break;
                q = zr2 + zi2 - rt2;             /* |z|^2 - rt^2, no sqrt   */
                if (q < 0.0f) q = -q;
                q *= 0.5f / rt;                  /* first-order -> ||z|-rt| */
                if (q < best) { best = q; bi = k; }
                zi = 2.0f * zr * zi + cim;
                zr = zr2 - zi2 + cre;
            }
            k = (int)(best * 512.0f);
            if (k >= 1024) { dst[x] = 0xFF000000u; continue; }
            iv = (int)p126_lut[k];
            if (iv < 6) { dst[x] = 0xFF000000u; continue; }
            col = p126_ramp[(bi * 23 + (int)(t * 0.55f)) & 255];
            hot = (iv * iv) >> 14;
            cr = (int)((((col >> 16) & 255) * iv) >> 10) + hot;
            cg = (int)((((col >> 8) & 255) * iv) >> 10) + hot;
            cb = (int)(((col & 255) * iv) >> 10) + hot;
            if (cr > 255) cr = 255; if (cg > 255) cg = 255; if (cb > 255) cb = 255;
            dst[x] = 0xFF000000u | ((uint32_t)cr << 16) |
                     ((uint32_t)cg << 8) | (uint32_t)cb;
        }
    }
    p126_blit(fb, w, h);
}
