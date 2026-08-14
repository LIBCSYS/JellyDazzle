/* 158 Blaze — Bridget Riley op-art: concentric rings of radial bands whose
 * angular phase zig-zags from ring to ring, so the flat page reads as a
 * twisted funnel. Built in log-polar: u = log(r)*k advances with time, giving
 * rings that stream inward forever and seamlessly (the pattern is periodic in
 * u, so nothing ever restarts); within ring floor(u) the fractional part f
 * shifts the band angle by +-amp*(f-0.5), the sign alternating per ring, which
 * is exactly Riley's zig. Bands are threshold-shaded with a width-aware
 * antialias term e = N*1.5/r_px, so edges soften automatically toward the
 * centre instead of aliasing. The starkest routine in the set: one accent hue
 * against black, high contrast, ~half the frame at zero — it will punch
 * straight through a MAX composite while leaving the other layers visible. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p158_up;

#define CW 480
#define CH 360

static unsigned char p158_img[CW * CH * 3];
static float p158_lr[CW * CH], p158_th[CW * CH], p158_rp[CW * CH];
static float p158_sin[4096];
static int *p158_xm;
static int p158_xmw;
static float p158_hueA[3], p158_hueB[3];
static float p158_hue0, p158_huew, p158_amp, p158_zoom, p158_spin;
static int p158_nsec;
static uint32_t p158_seedc;
static int p158_ready, p158_tabs;

static uint32_t p158_rs;
static float p158_rf(void)
{
    p158_rs ^= p158_rs << 13; p158_rs ^= p158_rs >> 17; p158_rs ^= p158_rs << 5;
    return (float)(p158_rs >> 8) * (1.0f / 16777216.0f);
}

static void p158_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static void p158_setup(uint32_t seed)
{
    int x, y, i;
    p158_rs = seed ? seed ^ 0xB1A2E5u : 0xB1A2E5u;
    p158_rf(); p158_rf();
    p158_hue0 = p158_rf();
    p158_huew = 0.03f + p158_rf() * 0.30f;
    p158_nsec = 9 + (int)(p158_rf() * 11.0f);
    p158_amp = 0.55f + p158_rf() * 0.85f;
    p158_zoom = (p158_rf() < 0.5f ? -1.0f : 1.0f) * (0.00085f + p158_rf() * 0.00075f);
    p158_spin = (p158_rf() - 0.5f) * 0.0013f;
    if (!p158_tabs) {
        for (i = 0; i < 4096; i++)
            p158_sin[i] = sinf((float)i * (6.2831853f / 4096.0f));
        for (y = 0; y < CH; y++) {
            float dy = (float)y + 0.5f - CH * 0.5f;
            for (x = 0; x < CW; x++) {
                float dx = ((float)x + 0.5f - CW * 0.5f) * 0.82f;
                float r = sqrtf(dx * dx + dy * dy);
                int o = y * CW + x;
                if (r < 0.9f) r = 0.9f;
                p158_rp[o] = r;
                p158_lr[o] = logf(r);
                p158_th[o] = atan2f(dy, dx);
            }
        }
        p158_tabs = 1;
    }
    p158_ready = 1;
    p158_seedc = seed;
}

static void p158_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p158_xmw != w) {
        free(p158_xm);
        p158_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p158_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p158_xmw = w;
    }
    jd_up_blit(&p158_up, fb, w, h, p158_img, CW, CH);
}

void pattern_158(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, kz, uof, spin, ns, amp;
    int x, y, c;
    (void)sl;
    if (!p158_ready || p158_seedc != seed) p158_setup(seed);
    p158_pal3(pal, p158_hue0, 0.94f, p158_hueA);
    p158_pal3(pal, p158_hue0 + p158_huew, 0.60f, p158_hueB);

    kz = 2.45f;
    uof = t * p158_zoom * 6.0f;
    spin = t * p158_spin;
    ns = (float)p158_nsec;
    amp = p158_amp * (0.72f + 0.28f * sinf(t * 0.00061f));

    for (y = 0; y < CH; y++) {
        const float *lrp = p158_lr + y * CW;
        const float *thp = p158_th + y * CW;
        const float *rpp = p158_rp + y * CW;
        unsigned char *op = p158_img + y * CW * 3;
        for (x = 0; x < CW; x++) {
            float u = lrp[x] * kz + uof;
            float fr = floorf(u), f = u - fr;
            float sgn = ((int)fr & 1) ? -1.0f : 1.0f;
            float a = thp[x] + amp * (f - 0.5f) * sgn / ns + spin;
            float v, e, band, ridge, val, mixc, dark, col[3];
            int idx = (int)(a * ns * (4096.0f / 6.2831853f)) & 4095;
            v = p158_sin[idx];
            e = ns * 1.9f / rpp[x];
            if (e > 1.0f) e = 1.0f;
            if (e < 0.02f) e = 0.02f;
            band = (v + e) / (2.0f * e);
            if (band < 0.0f) band = 0.0f; else if (band > 1.0f) band = 1.0f;
            band = band * band * (3.0f - 2.0f * band);
            ridge = 1.0f - fabsf(2.0f * f - 1.0f);
            ridge = 0.30f + 0.70f * ridge * ridge;
            val = band * ridge;
            dark = rpp[x] * (1.0f / (CH * 0.10f));
            if (dark > 1.0f) dark = 1.0f;
            val *= dark;
            mixc = 0.5f + 0.5f * v;
            for (c = 0; c < 3; c++) {
                float cc = p158_hueA[c] + (p158_hueB[c] - p158_hueA[c]) * mixc;
                float o = (cc * val + 0.35f * val * val * val) * 255.0f + 0.5f;
                col[c] = o <= 0.0f ? 0.0f : o >= 255.0f ? 255.0f : o;
            }
            op[x * 3] = (unsigned char)col[0];
            op[x * 3 + 1] = (unsigned char)col[1];
            op[x * 3 + 2] = (unsigned char)col[2];
        }
    }
    p158_blit(fb, w, h);
}
