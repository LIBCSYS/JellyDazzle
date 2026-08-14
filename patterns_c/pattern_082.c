/* 082 Greek Key — interlocking meander hooks in a gold-framed panel, ringed by
 * kaleidoscope wedges. Port of lab/patterns/082_greek_key/proto.py.
 * Repaint pattern, full resolution, one fast-atan2 per pixel. */
#include "../jellydazzle.h"
#include <math.h>

#define P82_TAU 6.28318530718f
#define P82_HSPAN 920.0f              /* palette entries per unit of proto hue */

static float p82_ptab[1024][3];
static float p82_cellh[4][6];
static float p82_bval[512];
static int p82_cellh_done;

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
                p82_cellh[gy][gx] = sinf((float)gx * 1.3f + (float)gy * 2.1f) * 0.06f;
        p82_cellh_done = 1;
    }
    /* border brightness LUT over dpan in [0,2) */
    for (x = 0; x < 512; x++)
        p82_bval[x] = 0.45f + 0.30f * sinf((float)x * (2.0f / 512.0f) * 6.0f
                                           - t * 0.015f);

    for (y = 0; y < h; y++) {
        float py = ((float)y + 0.5f) * isy - 120.0f;
        float apy = fabsf(py), dvy = apy * (1.0f / 80.0f);
        uint32_t *row = fb + (long)y * w;
        int inrow = (apy < 80.0f);
        for (x = 0; x < w; x++) {
            float px = ((float)x + 0.5f) * isx - 160.0f;
            float apx = fabsf(px), dpan;
            float hue, bright, cr, cg, cb;
            const float *c;
            int ir, ig, ib;

            dpan = apx * (1.0f / 120.0f); if (dvy > dpan) dpan = dvy;

            if (inrow && apx < 120.0f) {
                float fxp = px + 120.0f, fyp = py + 80.0f;
                float u, v, un, vn, r, a, s;
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
                s = r * 3.0f - a + crawl;
                s -= floorf(s);
                if (s < 0.5f) { hue = 0.02f; bright = 1.0f; }
                else          { hue = 0.55f; bright = 0.50f; }
                hue += p82_cellh[cgy][cgx] + rot;
            } else {
                float th = p82_atan2(py, px);
                float wed = th * (8.0f / P82_TAU) + wrot;
                int bi;
                wed -= floorf(wed);
                hue = floorf(wed * 4.0f) * 0.09f + 0.55f + rot;
                bi = (int)(dpan * 256.0f); if (bi > 511) bi = 511;
                bright = p82_bval[bi];
            }

            /* gold frame ring around the panel */
            if (fabsf(dpan - 1.0f) < 0.02f) {
                row[x] = 0xFFFFF299u;
                continue;
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
