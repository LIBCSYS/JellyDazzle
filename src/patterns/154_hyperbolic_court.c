/* 154 Hyperbolic Court — a {p,q} tiling of the Poincare disk, drawn as wire.
 * Per pixel the point is carried by a slowly drifting hyperbolic translation
 * z -> (z+a)/(1+conj(a)z), then folded back into the Schwarz triangle (2,p,q)
 * by reflection: the two mirrors through the origin fold the angle into the
 * wedge pi/p, and the third mirror — the circle orthogonal to the unit circle
 * with c = cos(pi/q)/sqrt(cos^2(pi/q) - sin^2(pi/p)), r = sqrt(c^2-1) — is
 * applied by inversion until the point stops moving. The Jacobian of the whole
 * fold is accumulated on the way, so the tile edges can be drawn at CONSTANT
 * screen width all the way to the ideal boundary, where infinitely many
 * shrinking cells crowd together. The drift never repeats: the tiling is
 * invariant, so the frame is a seamless endless glide. Dark cells, bright
 * seams — sparse enough to stack. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p154_up;

#define CW 400
#define CH 300

static unsigned char p154_img[CW * CH * 3];
static int *p154_xm;
static int p154_xmw;
static float p154_edgec[3], p154_fillc[24][3];
static float p154_hue0, p154_huew, p154_ax0, p154_spd;
static int p154_p, p154_q;
static float p154_cc, p154_rr, p154_nx, p154_ny;
static uint32_t p154_seedc;
static int p154_ready;

static uint32_t p154_rs;
static float p154_rf(void)
{
    p154_rs ^= p154_rs << 13; p154_rs ^= p154_rs >> 17; p154_rs ^= p154_rs << 5;
    return (float)(p154_rs >> 8) * (1.0f / 16777216.0f);
}

static void p154_pal3(const uint32_t *pal, float hue, float sat, float *o)
{
    uint32_t p; float r, g, b, mx;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
    mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    o[0] = (1.0f - sat) + sat * r / mx;
    o[1] = (1.0f - sat) + sat * g / mx;
    o[2] = (1.0f - sat) + sat * b / mx;
}

static void p154_setup(uint32_t seed)
{
    static const int pv[6] = {5, 4, 6, 7, 5, 8};
    static const int qv[6] = {4, 5, 4, 3, 5, 3};
    int k;
    float sp, cq, den, phi;
    p154_rs = seed ? seed ^ 0x7B0D15u : 0x7B0D15u;
    p154_rf(); p154_rf();
    k = (int)(p154_rf() * 6.0f); if (k > 5) k = 5;
    p154_p = pv[k]; p154_q = qv[k];
    phi = 3.14159265f / (float)p154_p;
    sp = sinf(phi);
    cq = cosf(3.14159265f / (float)p154_q);
    den = cq * cq - sp * sp;
    if (den < 0.004f) den = 0.004f;
    p154_cc = cq / sqrtf(den);
    p154_rr = sqrtf(p154_cc * p154_cc - 1.0f);
    p154_nx = -sp; p154_ny = cosf(phi);          /* normal of the pi/p mirror */
    p154_hue0 = p154_rf();
    p154_huew = 0.05f + p154_rf() * 0.36f;
    p154_ax0 = 0.34f + p154_rf() * 0.26f;
    p154_spd = (p154_rf() < 0.5f ? -1.0f : 1.0f) * (0.00075f + p154_rf() * 0.00065f);
    p154_ready = 1;
    p154_seedc = seed;
}

static void p154_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p154_xmw != w) {
        free(p154_xm);
        p154_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p154_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p154_xmw = w;
    }
    jd_up_blit(&p154_up, fb, w, h, p154_img, CW, CH);
}

void pattern_154(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, R, ax, ay, aa, rc, rs, wpx;
    int px, py, i;
    (void)sl;
    if (!p154_ready || p154_seedc != seed) p154_setup(seed);

    for (i = 0; i < 24; i++)
        p154_pal3(pal, p154_hue0 + p154_huew * ((float)i * (1.0f / 23.0f)),
                  0.95f, p154_fillc[i]);
    p154_pal3(pal, p154_hue0 + p154_huew * 0.5f, 0.45f, p154_edgec);

    R = (float)CH * 0.470f;
    {
        float ph = t * p154_spd;
        float m = p154_ax0 * (0.72f + 0.28f * sinf(t * 0.00043f));
        ax = m * cosf(ph); ay = m * sinf(ph);
    }
    aa = ax * ax + ay * ay;
    rc = cosf(t * 0.00055f); rs = sinf(t * 0.00055f);
    wpx = 1.15f;

    for (py = 0; py < CH; py++) {
        float y0 = ((float)py + 0.5f - CH * 0.5f) / R;
        unsigned char *op = p154_img + py * CW * 3;
        for (px = 0; px < CW; px++) {
            float x0 = ((float)px + 0.5f - CW * 0.5f) / R;
            float zx, zy, J, d, dmin, edge, fill, fade, e, out[3];
            float dr, di, dd, nrx, nry;
            int k, rounds, c;
            float qq = x0 * x0 + y0 * y0;
            if (qq >= 0.9985f) {
                op[px * 3] = 0; op[px * 3 + 1] = 0; op[px * 3 + 2] = 0;
                continue;
            }
            zx = x0 * rc - y0 * rs; zy = x0 * rs + y0 * rc;
            /* hyperbolic translation */
            nrx = zx + ax; nry = zy + ay;
            dr = 1.0f + ax * zx + ay * zy; di = ax * zy - ay * zx;
            dd = dr * dr + di * di;
            if (dd < 1e-8f) dd = 1e-8f;
            J = (1.0f - aa) / dd;
            zx = (nrx * dr + nry * di) / dd;
            zy = (nry * dr - nrx * di) / dd;

            for (k = 0; k < 26; k++) {
                int moved = 0;
                for (rounds = 0; rounds < 12; rounds++) {
                    int m2 = 0;
                    if (zy < 0.0f) { zy = -zy; m2 = 1; }
                    d = zx * p154_nx + zy * p154_ny;
                    if (d > 0.0f) {
                        zx -= 2.0f * d * p154_nx; zy -= 2.0f * d * p154_ny;
                        m2 = 1;
                    }
                    if (!m2) break;
                }
                {
                    float ex = zx - p154_cc, ey = zy;
                    float e2 = ex * ex + ey * ey;
                    if (e2 < p154_rr * p154_rr && e2 > 1e-9f) {
                        float s = p154_rr * p154_rr / e2;
                        zx = p154_cc + ex * s; zy = ey * s;
                        J *= s;
                        moved = 1;
                    }
                }
                if (!moved) break;
            }

            /* distances to the three mirrors, in fundamental-domain units */
            dmin = zy;
            d = -(zx * p154_nx + zy * p154_ny); if (d < dmin) dmin = d;
            {
                float ex = zx - p154_cc;
                d = sqrtf(ex * ex + zy * zy) - p154_rr;
                if (d < 0.0f) d = -d;
                if (d < dmin) dmin = d;
            }
            if (dmin < 0.0f) dmin = 0.0f;
            e = dmin / (J > 1e-9f ? J : 1e-9f) * R / wpx;
            edge = 1.0f / (1.0f + e * e);
            fade = 1.0f - qq;
            fade = fade * 12.0f; if (fade > 1.0f) fade = 1.0f;
            fade *= fade;
            fill = 0.16f + 0.13f * (float)((k * 5) & 3);
            {
                const float *fc = p154_fillc[(k * 7) % 24];
                float g = edge * (0.55f + 0.45f * edge);
                for (c = 0; c < 3; c++) {
                    float v = (fill * fc[c] + g * (p154_edgec[c] * 0.85f + 0.30f)) * fade;
                    v = v * 255.0f + 0.5f;
                    out[c] = v <= 0.0f ? 0.0f : v >= 255.0f ? 255.0f : v;
                }
            }
            op[px * 3] = (unsigned char)out[0];
            op[px * 3 + 1] = (unsigned char)out[1];
            op[px * 3 + 2] = (unsigned char)out[2];
        }
    }
    p154_blit(fb, w, h);
}
