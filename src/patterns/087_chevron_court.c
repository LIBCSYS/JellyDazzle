/* 087 Chevron Court — four-fold mirrored chevrons marching toward a cream-framed
 * striped core, with teal accent V's and corner ray fans.
 * Port of lab/patterns/087_chevron_court/proto.py. Repaint pattern: the whole
 * chevron body is a 1-D LUT over the L1 distance s = |x-cx| + |y-cy|, so the
 * inner loop is one table read plus an optional ray add. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P87_TAU 6.28318530718f
#define P87_HSPAN 1000.0f

static float p87_ptab[1024][3];
static float p87_sin[2048];
static float p87_body[1024][3];
static float p87_far[1024];
static float p87_ray[512];
static float p87_core[2048][3];
static int p87_inited;

static void p87_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p87_sin[i] = sinf((float)i * (P87_TAU / 2048.0f));
    p87_inited = 1;
}

static void p87_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 24.0f) mx = 24.0f;
        p87_ptab[i][0] = r / mx; p87_ptab[i][1] = g / mx; p87_ptab[i][2] = b / mx;
    }
}

static float p87_lsin(float a)
{
    return p87_sin[((int)(a * 325.949318f + 32768.5f)) & 2047];
}

static float p87_atan2(float y, float x)
{
    float ax = fabsf(x), ay = fabsf(y), a, s, r;
    if (ax < 1e-12f && ay < 1e-12f) return 0.0f;
    a = (ax > ay) ? ay / (ax + 1e-20f) : ax / (ay + 1e-20f);
    s = a * a;
    r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) r = 1.57079637f - r;
    return r;                                  /* first quadrant only */
}

void pattern_087(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float isx = 320.0f / (float)w, isy = 240.0f / (float)h;
    float chue;
    int i, x, y, hs;
    (void)sl; (void)seed;

    if (!p87_inited) p87_init();
    p87_buildpal(pal);

    /* chevron body LUT over s in [0,512) at 0.5 px steps */
    for (i = 0; i < 1024; i++) {
        float s = (float)i * 0.5f;
        float c1 = 0.5f + 0.5f * p87_lsin(s * 0.22f + t * 0.012f);
        float c2 = 0.5f + 0.5f * p87_lsin(s * 0.085f - t * 0.006f);
        float hue = 0.86f + 0.10f * p87_lsin(s * 0.018f - t * 0.004f);
        float c3 = (p87_lsin(s * 0.15f - t * 0.009f) - 0.62f) * (1.0f / 0.38f);
        float bri = 0.12f + 0.60f * c1 * c2, fr;
        const float *c = p87_ptab[(int)(hue * P87_HSPAN + 4096.0f) & 1023];
        if (c3 < 0.0f) c3 = 0.0f; if (c3 > 1.0f) c3 = 1.0f;
        p87_body[i][0] = c[0] * bri;
        p87_body[i][1] = c[1] * bri + c3 * 0.45f;
        p87_body[i][2] = c[2] * bri + c3 * 0.42f;
        fr = (s - 130.0f) * 0.01f; if (fr < 0.0f) fr = 0.0f; if (fr > 1.0f) fr = 1.0f;
        p87_far[i] = fr;
    }
    /* ray fan LUT over the first-quadrant angle */
    for (i = 0; i < 512; i++) {
        float th = (float)i * (1.570796327f / 512.0f);
        float r = (p87_lsin(th * 12.0f + t * 0.008f) - 0.90f) * 10.0f;
        if (r < 0.0f) r = 0.0f; if (r > 1.0f) r = 1.0f;
        p87_ray[i] = r;
    }
    /* core scanline ramp, indexed by proto row * 8 */
    chue = 0.38f + 0.05f * p87_lsin(t * 0.005f);
    hs = (int)(chue * P87_HSPAN + 4096.0f) & 1023;
    for (i = 0; i < 2048; i++) {
        float yy = (float)i * 0.125f;
        float st = 0.5f + 0.5f * p87_lsin(yy * 0.9f - t * 0.018f);
        float bri = 0.25f + 0.75f * st;
        p87_core[i][0] = p87_ptab[hs][0] * bri;
        p87_core[i][1] = p87_ptab[hs][1] * bri;
        p87_core[i][2] = p87_ptab[hs][2] * bri;
    }

    for (y = 0; y < h; y++) {
        float yy = ((float)y + 0.5f) * isy;
        float v = fabsf(yy - 120.0f);
        uint32_t *row = fb + (long)y * w;
        int ci = (int)(yy * 8.0f); if (ci > 2047) ci = 2047; if (ci < 0) ci = 0;
        for (x = 0; x < w; x++) {
            float u = fabsf(((float)x + 0.5f) * isx - 160.0f);
            float s = u + v, cr, cg, cb;
            int k, ir, ig, ib;

            if (u < 74.0f && v < 30.0f) {
                if (u < 70.0f && v < 26.0f) {
                    cr = p87_core[ci][0]; cg = p87_core[ci][1]; cb = p87_core[ci][2];
                } else {
                    cr = 0.95f; cg = 0.88f; cb = 0.70f;
                }
            } else {
                k = (int)(s * 2.0f); if (k > 1023) k = 1023;
                cr = p87_body[k][0]; cg = p87_body[k][1]; cb = p87_body[k][2];
                if (s > 130.0f) {
                    float fr = p87_far[k];
                    int ai = (int)(p87_atan2(v, u) * (512.0f / 1.570796327f));
                    float rv;
                    if (ai > 511) ai = 511; if (ai < 0) ai = 0;
                    rv = p87_ray[ai] * fr;
                    cr += rv * 0.5f; cg += rv * 0.35f; cb += rv * 0.1f;
                }
            }
            ir = (int)(cr * 255.0f); if (ir > 255) ir = 255; if (ir < 0) ir = 0;
            ig = (int)(cg * 255.0f); if (ig > 255) ig = 255; if (ig < 0) ig = 0;
            ib = (int)(cb * 255.0f); if (ib > 255) ib = 255; if (ib < 0) ib = 0;
            row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                   | ((uint32_t)ig << 8) | (uint32_t)ib;
        }
    }
}
