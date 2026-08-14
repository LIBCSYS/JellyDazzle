/* 084 Gear Rosettes — four toothed sunflower gears on a drifting diagonal
 * stripe ground, cyan diamond chain down the centre.
 * Port of lab/patterns/084_gear_rosettes/proto.py. Repaint, full resolution:
 * ground from a 1-D diagonal LUT, gears drawn inside their bounding boxes. */
#include "../jellydazzle.h"
#include <math.h>

#define P84_TAU 6.28318530718f
#define P84_HSPAN 1080.0f

static float p84_ptab[1024][3];
static float p84_sin[2048];
static uint8_t p84_gnd[1024][3];
static int p84_inited;

static const float p84_cx[4] = { 80.0f, 240.0f,  80.0f, 240.0f };
static const float p84_cy[4] = { 60.0f,  60.0f, 180.0f, 180.0f };

static void p84_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p84_sin[i] = sinf((float)i * (P84_TAU / 2048.0f));
    p84_inited = 1;
}

static void p84_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 24.0f) mx = 24.0f;
        p84_ptab[i][0] = r / mx; p84_ptab[i][1] = g / mx; p84_ptab[i][2] = b / mx;
    }
}

static float p84_lsin(float a)
{
    return p84_sin[((int)(a * 325.949318f + 32768.5f)) & 2047];
}

static float p84_atan2(float y, float x)
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

void pattern_084(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sx = (float)w / 320.0f, sy = (float)h / 240.0f;
    float isx = 1.0f / sx, isy = 1.0f / sy;
    int i, x, y;
    (void)sl; (void)seed;

    if (!p84_inited) p84_init();
    p84_buildpal(pal);

    /* ---- diagonal stripe ground LUT over s = (x+y)/2, s in [0,512) ---- */
    for (i = 0; i < 1024; i++) {
        float s = (float)i * 0.5f;
        float g = 0.5f + 0.5f * p84_lsin(s * 0.35f - t * 0.008f);
        float hg = 0.62f + 0.05f * p84_lsin(s * 0.04f + t * 0.002f);
        float bri = 0.18f + 0.22f * g;
        const float *c = p84_ptab[(int)(hg * P84_HSPAN + 4096.0f) & 1023];
        int k;
        for (k = 0; k < 3; k++) {
            int q = (int)(c[k] * bri * 255.0f);
            if (q > 255) q = 255; if (q < 0) q = 0;
            p84_gnd[i][k] = (uint8_t)q;
        }
    }
    for (y = 0; y < h; y++) {
        float py = ((float)y + 0.5f) * isy;
        uint32_t *row = fb + (long)y * w;
        for (x = 0; x < w; x++) {
            float s = (((float)x + 0.5f) * isx + py) * 0.5f;
            int k = (int)(s * 2.0f); if (k > 1023) k = 1023; if (k < 0) k = 0;
            row[x] = 0xFF000000u | ((uint32_t)p84_gnd[k][0] << 16)
                   | ((uint32_t)p84_gnd[k][1] << 8) | (uint32_t)p84_gnd[k][2];
        }
    }

    /* ---- four gear rosettes ---- */
    for (i = 0; i < 4; i++) {
        float rot = fmodf(t * 0.006f * ((i & 1) ? 1.0f : -1.0f) + (float)i * 0.7f,
                          P84_TAU);
        float hbase = 0.13f + 0.05f * p84_lsin(t * 0.004f + (float)i);
        float lim = 52.0f;
        int x0 = (int)((p84_cx[i] - lim) * sx), x1 = (int)((p84_cx[i] + lim) * sx) + 1;
        int y0 = (int)((p84_cy[i] - lim) * sy), y1 = (int)((p84_cy[i] + lim) * sy) + 1;
        if (x0 < 0) x0 = 0; if (x1 > w) x1 = w;
        if (y0 < 0) y0 = 0; if (y1 > h) y1 = h;
        for (y = y0; y < y1; y++) {
            float dy = ((float)y + 0.5f) * isy - p84_cy[i];
            float qy = dy * dy;
            uint32_t *row = fb + (long)y * w;
            for (x = x0; x < x1; x++) {
                float dx = ((float)x + 0.5f) * isx - p84_cx[i];
                float r = sqrtf(qy + dx * dx);
                float th, R, sd, hue, bri, rim, cr, cg, cb;
                const float *c;
                uint32_t p;
                int ir, ig, ib;
                if (r > lim) continue;
                th = p84_atan2(dy, dx);
                R = 44.0f + 5.0f * p84_lsin(16.0f * (th + rot));
                rim = 1.0f - fabsf(r - R) * 0.4f;
                if (rim < 0.0f) rim = 0.0f; if (rim > 1.0f) rim = 1.0f;
                if (r < R) {
                    sd = 0.5f + 0.5f * p84_lsin(r * 0.55f - t * 0.012f)
                                     * p84_lsin(8.0f * (th - rot * 2.0f));
                    hue = hbase + sd * 0.12f;
                    bri = 0.40f + 0.60f * sd;
                    c = p84_ptab[(int)(hue * P84_HSPAN + 4096.0f) & 1023];
                    cr = c[0] * bri; cg = c[1] * bri; cb = c[2] * bri;
                } else {
                    if (rim <= 0.0f) continue;
                    p = row[x];
                    cr = (float)((p >> 16) & 255) * (1.0f / 255.0f);
                    cg = (float)((p >> 8) & 255) * (1.0f / 255.0f);
                    cb = (float)(p & 255) * (1.0f / 255.0f);
                }
                cr += rim * 0.5f; cg += rim * 0.5f; cb += rim * 0.2f;
                ir = (int)(cr * 255.0f); if (ir > 255) ir = 255; if (ir < 0) ir = 0;
                ig = (int)(cg * 255.0f); if (ig > 255) ig = 255; if (ig < 0) ig = 0;
                ib = (int)(cb * 255.0f); if (ib > 255) ib = 255; if (ib < 0) ib = 0;
                row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                       | ((uint32_t)ig << 8) | (uint32_t)ib;
            }
        }
    }

    /* ---- mirrored diamond chain on the centre vertical ---- */
    {
        int x0 = (int)((160.0f - 9.5f) * sx), x1 = (int)((160.0f + 9.5f) * sx) + 1;
        if (x0 < 0) x0 = 0; if (x1 > w) x1 = w;
        for (y = 0; y < h; y++) {
            float py = ((float)y + 0.5f) * isy + t * 0.05f;
            float m = py - 30.0f * floorf(py * (1.0f / 30.0f)) - 15.0f;
            float am = fabsf(m);
            uint32_t *row = fb + (long)y * w;
            if (am > 9.0f) continue;
            for (x = x0; x < x1; x++) {
                float dcol = fabsf(((float)x + 0.5f) * isx - 160.0f);
                float dm = am + dcol;
                float cm = 1.0f - dm * (1.0f / 9.0f);
                uint32_t p;
                int ir, ig, ib;
                if (cm <= 0.0f) continue;
                p = row[x];
                ir = (int)((p >> 16) & 255) + (int)(cm * 0.15f * 255.0f);
                ig = (int)((p >> 8) & 255) + (int)(cm * 0.50f * 255.0f);
                ib = (int)(p & 255) + (int)(cm * 0.55f * 255.0f);
                if (ir > 255) ir = 255; if (ig > 255) ig = 255; if (ib > 255) ib = 255;
                row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                       | ((uint32_t)ig << 8) | (uint32_t)ib;
            }
        }
    }
}
