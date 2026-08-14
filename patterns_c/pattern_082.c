/* 082 Greek Key — interlocking meander hooks in a gold-framed panel, ringed by
 * kaleidoscope wedges. Port of lab/patterns/082_greek_key/proto.py.
 * Repaint pattern, full resolution, one fast-atan2 per pixel. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P82_TAU 6.28318530718f
#define P82_HSPAN 920.0f              /* palette entries per unit of proto hue */

static float p82_ptab[1024][3];
static float p82_cellh[24];
static float p82_bval[512];
static int p82_cellh_done;

/* Geometry cache. Nothing the atan2 feeds depends on the frame: the meander
 * phase (r*3 - a), the wedge angle (th*8/TAU), the panel/border classification
 * and the border LUT index are all pure functions of (x,y). Bake them once per
 * resolution; only the additive drifts (crawl/rot/wrot) move per frame.
 *   p82_geo : the frame-invariant phase, in turns
 *   p82_inf : bit15 = gold frame ring, bit14 = inside panel,
 *             low bits  = cell index 0..23 (panel) or p82_bval index (border) */
static float    *p82_geo;
static uint16_t *p82_inf;
static int p82_tw, p82_th;
static float    p82_georow[4096];        /* fallback if the alloc ever fails */
static uint16_t p82_infrow[4096];

static void p82_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 24.0f) mx = 24.0f;
        p82_ptab[i][0] = r / mx; p82_ptab[i][1] = g / mx; p82_ptab[i][2] = b / mx;
    }
}

static float p82_atan2(float y, float x)
{
    float ax = fabsf(x), ay = fabsf(y), a, s, r;
    if (ax < 1e-12f && ay < 1e-12f) return 0.0f;
    a = (ax > ay) ? ay / (ax + 1e-20f) : ax / (ay + 1e-20f);
    s = a * a;
    r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) r = 1.57079637f - r;
    if (x < 0.0f) r = 3.14159274f - r;
    if (y < 0.0f) r = -r;
    return r;
}

/* One row of the geometry cache. Every expression here is lifted verbatim from
 * the inner loop it replaces, so the rendered image is unchanged. */
static void p82_georaw(float *geo, uint16_t *inf, float py, float isx, int w)
{
    float apy = fabsf(py), dvy = apy * (1.0f / 80.0f);
    int inrow = (apy < 80.0f), x;
    if (w > 4096) w = 4096;
    for (x = 0; x < w; x++) {
        float px = ((float)x + 0.5f) * isx - 160.0f;
        float apx = fabsf(px), dpan;
        uint16_t bits;

        dpan = apx * (1.0f / 120.0f); if (dvy > dpan) dpan = dvy;

        if (inrow && apx < 120.0f) {
            float fxp = px + 120.0f, fyp = py + 80.0f;
            float u, v, un, vn, r, a;
            int cgx = (int)(fxp * (1.0f / 40.0f));
            int cgy = (int)(fyp * (1.0f / 40.0f));
            if (cgx > 5) cgx = 5;
            if (cgy > 3) cgy = 3;
            u = fxp - (float)cgx * 40.0f - 20.0f;
            v = fyp - (float)cgy * 40.0f - 20.0f;
            un = u * 0.05f;
            vn = v * 0.05f;
            if ((cgx + cgy) & 1) vn = -vn;
            r = fabsf(un); { float av = fabsf(vn); if (av > r) r = av; }
            a = p82_atan2(vn, un) * (1.0f / P82_TAU);
            geo[x] = r * 3.0f - a;
            bits = (uint16_t)(0x4000u | (unsigned)(cgy * 6 + cgx));
        } else {
            int bi;
            geo[x] = p82_atan2(py, px) * (8.0f / P82_TAU);
            bi = (int)(dpan * 256.0f); if (bi > 511) bi = 511;
            bits = (uint16_t)bi;
        }
        if (fabsf(dpan - 1.0f) < 0.02f) bits |= 0x8000u;
        inf[x] = bits;
    }
}

static void p82_map(int w, int h)
{
    float isx, isy;
    int y;
    if (p82_tw == w && p82_th == h && p82_geo) return;
    free(p82_geo); free(p82_inf);
    p82_geo = (float *)malloc(sizeof(float) * (size_t)w * (size_t)h);
    p82_inf = (uint16_t *)malloc(sizeof(uint16_t) * (size_t)w * (size_t)h);
    if (!p82_geo || !p82_inf) {
        free(p82_geo); free(p82_inf); p82_geo = 0; p82_inf = 0;
        p82_tw = 0; p82_th = 0; return;
    }
    isx = 320.0f / (float)w; isy = 240.0f / (float)h;
    for (y = 0; y < h; y++)
        p82_georaw(p82_geo + (long)y * w, p82_inf + (long)y * w,
                   ((float)y + 0.5f) * isy - 120.0f, isx, w);
    p82_tw = w; p82_th = h;
}

void pattern_082(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float isx = 320.0f / (float)w, isy = 240.0f / (float)h;
    float crawl = t * 0.004f, rot = t * 0.0015f, wrot = t * 0.003f;
    int gx, gy, x, y;
    (void)sl; (void)seed;

    p82_buildpal(pal);
    if (!p82_cellh_done) {
        for (gy = 0; gy < 4; gy++)
            for (gx = 0; gx < 6; gx++)
                p82_cellh[gy * 6 + gx] = sinf((float)gx * 1.3f
                                              + (float)gy * 2.1f) * 0.06f;
        p82_cellh_done = 1;
    }
    p82_map(w, h);
    /* border brightness LUT over dpan in [0,2) */
    for (x = 0; x < 512; x++)
        p82_bval[x] = 0.45f + 0.30f * sinf((float)x * (2.0f / 512.0f) * 6.0f
                                           - t * 0.015f);

    for (y = 0; y < h; y++) {
        float py = ((float)y + 0.5f) * isy - 120.0f;
        uint32_t *row = fb + (long)y * w;
        const float *geo; const uint16_t *inf;
        if (p82_geo) { geo = p82_geo + (long)y * w; inf = p82_inf + (long)y * w; }
        else { p82_georaw(p82_georow, p82_infrow, py, isx, w);
               geo = p82_georow; inf = p82_infrow; }
        for (x = 0; x < w; x++) {
            unsigned bits = inf[x];
            float hue, bright, cr, cg, cb;
            const float *c;
            int ir, ig, ib;

            /* gold frame ring around the panel */
            if (bits & 0x8000u) { row[x] = 0xFFFFF299u; continue; }

            if (bits & 0x4000u) {
                float s = geo[x] + crawl;
                s -= floorf(s);
                if (s < 0.5f) { hue = 0.02f; bright = 1.0f; }
                else          { hue = 0.55f; bright = 0.50f; }
                hue += p82_cellh[bits & 31u] + rot;
            } else {
                float wed = geo[x] + wrot;
                wed -= floorf(wed);
                hue = floorf(wed * 4.0f) * 0.09f + 0.55f + rot;
                bright = p82_bval[bits & 511u];
            }

            c = p82_ptab[(int)(hue * P82_HSPAN + 4096.0f) & 1023];
            cr = c[0] * bright; cg = c[1] * bright; cb = c[2] * bright;
            ir = (int)(cr * 255.0f); if (ir > 255) ir = 255; if (ir < 0) ir = 0;
            ig = (int)(cg * 255.0f); if (ig > 255) ig = 255; if (ig < 0) ig = 0;
            ib = (int)(cb * 255.0f); if (ib > 255) ib = 255; if (ib < 0) ib = 0;
            row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                   | ((uint32_t)ig << 8) | (uint32_t)ib;
        }
    }
}
