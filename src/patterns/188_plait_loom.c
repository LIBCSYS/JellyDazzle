/* 188 Plait Loom — a woven basket wrapped onto a logarithmic spiral. Two
 * families of strands are defined by u = A + B and v = A - B, where
 * A = k log r + drift and B = n theta/2pi + spin; each family is therefore a
 * set of logarithmic spirals of opposite chirality, and they cross at every
 * lattice cell. A strand exists where |frac - 1/2| < halfwidth and is shaded
 * as a cylinder, sqrt(1 - (d/hw)^2), so each ribbon has a lit crown and dark
 * shoulders. Which ribbon is on top is decided the way a real plait decides it:
 * the parity of (floor u + floor v), which alternates over-under-over along
 * every strand and makes the weave interlock instead of merely overlap. n is
 * even, so the parity survives the theta seam and the weave closes exactly
 * around the ring. Edge softness is taken from the true gradient |grad u|,
 * which grows as 1/r, so the ribbons antialias themselves as they crowd toward
 * the middle and dissolve into a dark core instead of aliasing. Drift pulls the
 * whole plait inward forever; nothing restarts. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p188_up;

#define CW 480
#define CH 360

static unsigned char p188_img[CW * CH * 3];
static float p188_A0[CW * CH], p188_B0[CW * CH], p188_E[CW * CH], p188_CO[CW * CH];
static int *p188_xm;
static int p188_xmw;
static float p188_kz, p188_nn, p188_hw, p188_drift, p188_spin, p188_hue0, p188_huew;
static float p188_glw, p188_gk, p188_sin[1024];
static int p188_N;
static uint32_t p188_seedc;
static int p188_ready, p188_tabs;

static uint32_t p188_rs;
static float p188_rf(void)
{
    p188_rs ^= p188_rs << 13; p188_rs ^= p188_rs >> 17; p188_rs ^= p188_rs << 5;
    return (float)(p188_rs >> 8) * (1.0f / 16777216.0f);
}

static void p188_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static void p188_setup(uint32_t seed)
{
    int x, y, i;
    p188_rs = seed ? seed ^ 0x71A177u : 0x71A177u;
    p188_rf(); p188_rf();
    p188_kz = 1.9f + p188_rf() * 1.7f;
    p188_nn = (float)(2 * (5 + (int)(p188_rf() * 5.0f)));   /* even: 10..28 */
    p188_hw = 0.215f + p188_rf() * 0.055f;
    p188_drift = (p188_rf() < 0.5f ? -1.0f : 1.0f) * (0.0016f + p188_rf() * 0.0018f);
    p188_spin  = (p188_rf() < 0.5f ? -1.0f : 1.0f) * (0.00047f + p188_rf() * 0.00062f);
    p188_hue0 = p188_rf();
    p188_huew = 0.30f + p188_rf() * 0.68f;
    p188_glw = 0.0055f + p188_rf() * 0.0060f;
    p188_N = (int)p188_nn;
    p188_gk = 1024.0f * 3.0f / p188_nn;   /* glow period divides the seam jump */
    for (y = 0; y < CH; y++)
        for (x = 0; x < CW; x++) {
            float dx = ((float)x + 0.5f - CW * 0.5f) * 1.02f;
            float dy = (float)y + 0.5f - CH * 0.5f;
            float r = sqrtf(dx * dx + dy * dy), th, gu;
            int o = y * CW + x;
            if (r < 3.0f) r = 3.0f;
            th = atan2f(dy, dx);
            p188_A0[o] = logf(r) * p188_kz;
            p188_B0[o] = th * (p188_nn * (1.0f / 6.2831853f));
            gu = sqrtf(p188_kz * p188_kz + p188_nn * p188_nn * 0.02533f) / r;
            p188_E[o] = gu * 1.15f + 0.004f;
            p188_CO[o] = (r - 20.0f) * (1.0f / 46.0f);
            if (p188_CO[o] < 0.0f) p188_CO[o] = 0.0f;
            else if (p188_CO[o] > 1.0f) p188_CO[o] = 1.0f;
        }
    if (!p188_tabs) {
        for (i = 0; i < 1024; i++)
            p188_sin[i] = sinf((float)i * (6.2831853f / 1024.0f));
        p188_tabs = 1;
    }
    p188_ready = 1;
    p188_seedc = seed;
}

static void p188_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p188_xmw != w) {
        free(p188_xm);
        p188_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p188_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p188_xmw = w;
    }
    jd_up_blit(&p188_up, fb, w, h, p188_img, CW, CH);
}

void pattern_188(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, dt, st, hw, ihw, col[64][3], gph, gk;
    float ca_[32][3], cb_[32][3];
    int o, k, N;
    (void)sl;
    if (!p188_ready || p188_seedc != seed) p188_setup(seed);

    for (k = 0; k < 64; k++)
        p188_pal3(pal, p188_hue0 + p188_huew * ((float)k * (1.0f / 64.0f)), 0.92f, col[k]);
    N = p188_N;
    for (k = 0; k < N; k++) {
        int a = (k * 7) % N, b = (k * 11 + 3) % N;
        int i;
        for (i = 0; i < 3; i++) {
            ca_[k][i] = col[(a * 64 / N) & 63][i];
            cb_[k][i] = col[((b * 64 / N) + 32) & 63][i];
        }
    }
    gk = p188_gk;
    dt = t * p188_drift;
    st = t * p188_spin;
    hw = p188_hw + 0.028f * p188_sin[(int)(t * 0.00061f * 163.0f) & 1023];
    ihw = 1.0f / hw;
    gph = t * p188_glw * 163.0f;

    for (o = 0; o < CW * CH; o++) {
        float A = p188_A0[o] + dt, B = p188_B0[o] + st;
        float u = A + B, v = A - B, e = p188_E[o];
        float fu, fv, du, dv, ta, tb, sa, sb, r, g, b, co = p188_CO[o];
        float ua, va;
        int iu, iv, top;
        unsigned char *dp = p188_img + o * 3;
        ua = floorf(u); va = floorf(v);
        iu = (int)ua; iv = (int)va;
        fu = u - ua; fv = v - va;
        du = fabsf(fu - 0.5f); dv = fabsf(fv - 0.5f);
        ta = (hw - du) / e; if (ta > 1.0f) ta = 1.0f; else if (ta < 0.0f) ta = 0.0f;
        tb = (hw - dv) / e; if (tb > 1.0f) tb = 1.0f; else if (tb < 0.0f) tb = 0.0f;
        if (ta <= 0.0f && tb <= 0.0f) { dp[0] = dp[1] = dp[2] = 0; continue; }
        sa = 1.0f - du * ihw * du * ihw; if (sa < 0.0f) sa = 0.0f;
        sb = 1.0f - dv * ihw * dv * ihw; if (sb < 0.0f) sb = 0.0f;
        sa = sqrtf(sa); sb = sqrtf(sb);
        top = ((iu + iv) & 1) == 0;
        r = g = b = 0.0f;
        {
            const float *ca = ca_[((iu % N) + N) % N];
            const float *cb = cb_[((iv % N) + N) % N];
            float ga = 0.58f + 0.42f * p188_sin[(int)(v * gk - gph) & 1023];
            float gb = 0.58f + 0.42f * p188_sin[(int)(u * gk + gph) & 1023];
            float wa = ta * sa * ga, wb = tb * sb * gb;
            if (top) wb *= (1.0f - ta);
            else     wa *= (1.0f - tb);
            r = ca[0] * wa + cb[0] * wb;
            g = ca[1] * wa + cb[1] * wb;
            b = ca[2] * wa + cb[2] * wb;
        }
        r *= co; g *= co; b *= co;
        {
            int ri = (int)(r * 262.0f), gi = (int)(g * 262.0f), bi = (int)(b * 262.0f);
            dp[0] = (unsigned char)(ri > 255 ? 255 : ri < 0 ? 0 : ri);
            dp[1] = (unsigned char)(gi > 255 ? 255 : gi < 0 ? 0 : gi);
            dp[2] = (unsigned char)(bi > 255 ? 255 : bi < 0 ? 0 : bi);
        }
    }
    p188_blit(fb, w, h);
}
