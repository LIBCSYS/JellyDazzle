/* 086 Gem Orbit — nine faceted hexagonal gems (one large centre, eight on an
 * elliptical orbit) gliding over a deep-blue pond of breathing rings.
 * Port of lab/patterns/086_gem_orbit/proto.py. Repaint pattern, full
 * resolution: ground from a radial LUT, gems drawn per bounding box with a
 * static hex-radius table and 54 per-frame facet scalars. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P86_TAU 6.28318530718f
#define P86_NG 9
#define P86_HSPAN 820.0f

static float p86_ptab[1024][3];
static float p86_hexinv[1024];
static float p86_sin[2048];
static uint8_t p86_gnd[1024][3];
static float p86_fbri[P86_NG][6];
static const float *p86_fcol[P86_NG][6];
static int p86_inited;

static void p86_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p86_sin[i] = sinf((float)i * (P86_TAU / 2048.0f));
    for (i = 0; i < 1024; i++) {
        float th = (float)i * (P86_TAU / 1024.0f);
        float seg = P86_TAU / 6.0f;
        float a = th - seg * floorf(th / seg) - seg * 0.5f;
        p86_hexinv[i] = cosf(a) / cosf(3.14159265f / 6.0f);   /* 1 / ngon_r */
    }
    p86_inited = 1;
}

static void p86_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 24.0f) mx = 24.0f;
        p86_ptab[i][0] = r / mx; p86_ptab[i][1] = g / mx; p86_ptab[i][2] = b / mx;
    }
}

static float p86_lsin(float a)
{
    return p86_sin[((int)(a * 325.949318f + 32768.5f)) & 2047];
}

static float p86_atan2(float y, float x)
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

void pattern_086(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sx = (float)w / 320.0f, sy = (float)h / 240.0f;
    float isx = 1.0f / sx, isy = 1.0f / sy;
    float gcx[P86_NG], gcy[P86_NG], gspin[P86_NG], ginv[P86_NG], glim[P86_NG];
    float orb = t * 0.0025f, hdrift = t * 0.0015f;
    int k, f, i, x, y;
    (void)sl; (void)seed;

    if (!p86_inited) p86_init();
    p86_buildpal(pal);

    /* ---- ground ---- */
    for (i = 0; i < 1024; i++) {
        float fv = 0.5f + 0.5f * p86_lsin((float)i * 0.25f * 0.09f - t * 0.007f);
        float c[3];
        c[0] = fv * 0.05f + 0.02f; c[1] = fv * 0.07f + 0.02f; c[2] = fv * 0.16f + 0.08f;
        for (f = 0; f < 3; f++) {
            int q = (int)(c[f] * 255.0f); if (q > 255) q = 255; if (q < 0) q = 0;
            p86_gnd[i][f] = (uint8_t)q;
        }
    }
    for (y = 0; y < h; y++) {
        float py = ((float)y + 0.5f) * isy - 120.0f;
        float qy = py * py;
        uint32_t *row = fb + (long)y * w;
        for (x = 0; x < w; x++) {
            float px = ((float)x + 0.5f) * isx - 160.0f;
            int q = (int)(sqrtf(qy + px * px) * 4.0f); if (q > 1023) q = 1023;
            row[x] = 0xFF000000u | ((uint32_t)p86_gnd[q][0] << 16)
                   | ((uint32_t)p86_gnd[q][1] << 8) | (uint32_t)p86_gnd[q][2];
        }
    }

    /* ---- gem placement + per-facet shading ---- */
    for (k = 0; k < P86_NG; k++) {
        float R;
        if (k < 8) {
            float a = orb + (float)k * 0.785398163f;
            gcx[k] = 160.0f + 96.0f * p86_lsin(a + 1.570796327f);
            gcy[k] = 120.0f + 72.0f * p86_lsin(a);
            R = 17.0f;
        } else {
            gcx[k] = 160.0f; gcy[k] = 120.0f; R = 27.0f;
        }
        ginv[k] = 1.0f / R;
        glim[k] = R * 1.10f + 2.0f;
        gspin[k] = fmodf(t * 0.008f * ((k & 1) ? 1.0f : -1.0f) + (float)k, P86_TAU);
        if (gspin[k] < 0.0f) gspin[k] += P86_TAU;
        for (f = 0; f < 6; f++) {
            float hue = (float)k * 0.117f + (float)f * 0.045f + hdrift;
            p86_fbri[k][f] = 0.55f + 0.35f * p86_lsin((float)f * 2.1f + t * 0.010f
                                                      + (float)k * 1.7f);
            p86_fcol[k][f] = p86_ptab[(int)(hue * P86_HSPAN + 4096.0f) & 1023];
        }
    }

    /* ---- gems ---- */
    for (k = 0; k < P86_NG; k++) {
        float lim = glim[k], cx = gcx[k], cy = gcy[k], iv = ginv[k], sp = gspin[k];
        int x0 = (int)((cx - lim) * sx), x1 = (int)((cx + lim) * sx) + 1;
        int y0 = (int)((cy - lim) * sy), y1 = (int)((cy + lim) * sy) + 1;
        if (x0 < 0) x0 = 0; if (x1 > w) x1 = w;
        if (y0 < 0) y0 = 0; if (y1 > h) y1 = h;
        for (y = y0; y < y1; y++) {
            float dy = ((float)y + 0.5f) * isy - cy;
            float qy = dy * dy;
            uint32_t *row = fb + (long)y * w;
            for (x = x0; x < x1; x++) {
                float dx = ((float)x + 0.5f) * isx - cx;
                float r = sqrtf(qy + dx * dx) + 1e-6f;
                float th, d, e, s, cr, cg, cb;
                const float *c;
                int ai, fi, ir, ig, ib;
                if (r > lim) continue;
                th = p86_atan2(dy, dx) + sp;
                ai = (int)(th * (1024.0f / P86_TAU) + 1024.5f) & 1023;
                d = r * iv * p86_hexinv[ai];
                if (d >= 1.10f) continue;
                fi = (ai * 6) >> 10;
                c = p86_fcol[k][fi];
                s = 1.3f - d; if (s < 0.0f) s = 0.0f; if (s > 1.0f) s = 1.0f;
                s *= p86_fbri[k][fi];
                e = 1.0f - fabsf(d - 1.0f) * 10.0f;
                if (e < 0.0f) e = 0.0f; if (e > 1.0f) e = 1.0f;
                if (d >= 1.0f && e <= 0.03f) continue;
                cr = c[0] * s + e * 0.85f;
                cg = c[1] * s + e * 0.85f;
                cb = c[2] * s + e * 0.85f;
                ir = (int)(cr * 255.0f); if (ir > 255) ir = 255; if (ir < 0) ir = 0;
                ig = (int)(cg * 255.0f); if (ig > 255) ig = 255; if (ig < 0) ig = 0;
                ib = (int)(cb * 255.0f); if (ib > 255) ib = 255; if (ib < 0) ib = 0;
                row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                       | ((uint32_t)ig << 8) | (uint32_t)ib;
            }
        }
    }
}
