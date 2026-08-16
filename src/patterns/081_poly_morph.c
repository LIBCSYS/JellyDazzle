/* 081 Poly Morph — five glowing medallions melting triangle->square->pentagon
 * ->hexagon over a dim violet ring-field.
 * Port of lab/patterns/081_poly_morph/proto.py. Full-res repaint pattern:
 * ground painted from a radial ring LUT, each medallion drawn inside its own
 * bounding box with a per-frame 512-entry blended n-gon reciprocal table. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P81_NM 5
#define P81_TAU 6.28318530718f

static float p81_ptab[1024][3];
static float p81_inv[P81_NM][2048];
static uint8_t p81_ring[1024][3];
static float p81_pw[1025];
static int p81_inited;

static const float p81_cx[P81_NM] = { 160.0f,  62.0f, 258.0f,  62.0f, 258.0f };
static const float p81_cy[P81_NM] = { 120.0f,  60.0f,  60.0f, 180.0f, 180.0f };
static const float p81_R [P81_NM] = {  46.0f,  30.0f,  30.0f,  30.0f,  30.0f };
static const float p81_ph[P81_NM] = {   0.0f,   1.5f,   3.0f,   4.5f,   6.0f };

static void p81_init(void)
{
    int i;
    for (i = 0; i <= 1024; i++) p81_pw[i] = powf((float)i / 1024.0f, 0.7f);
    p81_inited = 1;
}

/* palette hue, brightness-normalized (proto's cosine rainbow stand-in) */
static void p81_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 24.0f) mx = 24.0f;
        p81_ptab[i][0] = r / mx; p81_ptab[i][1] = g / mx; p81_ptab[i][2] = b / mx;
    }
}

static float p81_atan2(float y, float x)
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

static float p81_ngon(float th, int n)
{
    float seg = P81_TAU / (float)n;
    float a = th - seg * floorf(th / seg) - seg * 0.5f;
    return cosf(3.14159265f / (float)n) / cosf(a);
}

void pattern_081(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sx = (float)w / 320.0f, sy = (float)h / 240.0f;
    float isx = 1.0f / sx, isy = 1.0f / sy;
    float spin[P81_NM];
    int i, x, y;
    (void)sl; (void)seed;

    if (!p81_inited) p81_init();
    p81_buildpal(pal);

    /* ground ring LUT: r0 in [0,256) at 0.25 px steps */
    for (i = 0; i < 1024; i++) {
        float f = 0.10f + 0.06f * sinf((float)i * 0.25f * 0.05f - t * 0.010f);
        float r = f * 0.35f, g = f * 0.22f, b = f * 0.65f;
        p81_ring[i][0] = (uint8_t)(r * 255.0f);
        p81_ring[i][1] = (uint8_t)(g * 255.0f);
        p81_ring[i][2] = (uint8_t)(b * 255.0f);
    }

    /* per-medallion morph tables */
    for (i = 0; i < P81_NM; i++) {
        float s = fmodf(t * 0.0035f + p81_ph[i] * 0.7f, 4.0f);
        int n1, n2, k;
        float m, R = p81_R[i];
        if (s < 0.0f) s += 4.0f;
        n1 = 3 + (int)s; n2 = (n1 < 6) ? n1 + 1 : 3;
        m = s - (float)(int)s;
        m = m * m * (3.0f - 2.0f * m);
        for (k = 0; k < 2048; k++) {
            float th = (float)k * (P81_TAU / 2048.0f);
            float rr = (1.0f - m) * p81_ngon(th, n1) + m * p81_ngon(th, n2);
            p81_inv[i][k] = 1.0f / (R * rr);
        }
        spin[i] = fmodf(t * 0.004f * ((i & 1) ? -1.0f : 1.0f) + p81_ph[i], P81_TAU);
        if (spin[i] < 0.0f) spin[i] += P81_TAU;
    }

    /* ---- ground ---- */
    for (y = 0; y < h; y++) {
        float py = ((float)y + 0.5f) * isy - 120.0f;
        float qy = py * py;
        uint32_t *row = fb + (long)y * w;
        for (x = 0; x < w; x++) {
            float px = ((float)x + 0.5f) * isx - 160.0f;
            float r0 = sqrtf(qy + px * px);
            int k = (int)(r0 * 4.0f); if (k > 1023) k = 1023;
            row[x] = 0xFF000000u | ((uint32_t)p81_ring[k][0] << 16)
                   | ((uint32_t)p81_ring[k][1] << 8) | (uint32_t)p81_ring[k][2];
        }
    }

    /* ---- medallions ---- */
    for (i = 0; i < P81_NM; i++) {
        float R = p81_R[i], lim = R * 1.09f + 2.0f;
        float hoff = (float)i * 0.19f + t * 0.0012f;
        int x0 = (int)((p81_cx[i] - lim) * sx), x1 = (int)((p81_cx[i] + lim) * sx) + 1;
        int y0 = (int)((p81_cy[i] - lim) * sy), y1 = (int)((p81_cy[i] + lim) * sy) + 1;
        const float *iv = p81_inv[i];
        float sp = spin[i];
        if (x0 < 0) x0 = 0; if (x1 > w) x1 = w;
        if (y0 < 0) y0 = 0; if (y1 > h) y1 = h;
        for (y = y0; y < y1; y++) {
            float dy = ((float)y + 0.5f) * isy - p81_cy[i];
            float qy = dy * dy;
            uint32_t *row = fb + (long)y * w;
            for (x = x0; x < x1; x++) {
                float dx = ((float)x + 0.5f) * isx - p81_cx[i];
                float r = sqrtf(qy + dx * dx) + 1e-6f;
                float th, d, sh, e, cr, cg, cb, hue;
                const float *c;
                int k, ir, ig, ib;
                th = p81_atan2(dy, dx) + sp;
                k = (int)(th * (2048.0f / P81_TAU) + 2048.5f) & 2047;
                d = r * iv[k];
                if (d >= 1.0817f) continue;
                hue = d * 0.9f + hoff;
                sh = 1.15f - d; if (sh < 0.0f) sh = 0.0f; if (sh > 1.0f) sh = 1.0f;
                sh = p81_pw[(int)(sh * 1024.0f)];
                e = 1.0f - fabsf(d - 1.0f) * 12.0f;
                if (e < 0.0f) e = 0.0f; if (e > 1.0f) e = 1.0f;
                c = p81_ptab[(int)(hue * 410.0f + 4096.0f) & 1023];
                cr = c[0] * sh + e * 0.9f;
                cg = c[1] * sh + e * 0.9f;
                cb = c[2] * sh + e * 1.0f;
                ir = (int)(cr * 255.0f); if (ir > 255) ir = 255; if (ir < 0) ir = 0;
                ig = (int)(cg * 255.0f); if (ig > 255) ig = 255; if (ig < 0) ig = 0;
                ib = (int)(cb * 255.0f); if (ib > 255) ib = 255; if (ib < 0) ib = 0;
                row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                       | ((uint32_t)ig << 8) | (uint32_t)ib;
            }
        }
    }
}
