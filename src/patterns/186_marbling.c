/* 186 Marbling — Turkish ebru, run backwards. Marbling has exact closed-form
 * maps, and both are inverted here per pixel instead of simulated. A DROP of
 * radius r at c pushes the whole sheet radially outward,
 *   P' = c + (P-c) sqrt(1 + r^2/|P-c|^2),
 * whose inverse is |P| = sqrt(|P'|^2 - r^2) — and the disc of radius r has no
 * preimage at all, which is precisely why fresh ink lands as a clean round
 * island that shoulders the older pattern aside. A TINE (the comb stroke) is
 *   P' = P + u * lambda / (1 + (z/beta)^2),  z = distance from the tine line,
 * and since z is unchanged by a displacement parallel to the line, its inverse
 * is exact too. Four drops and three combs are inverted in reverse order back
 * onto a full-spectrum banded ground, so every pixel is one chain of algebra
 * with no iteration and no error. Drop centres, comb amplitudes and the ground
 * phase all creep on detuned slow sines, so the sheet flows like liquid.
 * Dense, full-bleed, high-chroma: this is a GROUND layer, meant to be the thing
 * the sparse routines sit on top of. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p186_up;

#define CW 480
#define CH 360
#define NDROP 4
#define NTINE 3

static unsigned char p186_img[CW * CH * 3];
static int *p186_xm;
static int p186_xmw;
static float p186_dcx[NDROP], p186_dcy[NDROP], p186_dr2[NDROP];
static float p186_dph[NDROP * 2], p186_dhu[NDROP];
static float p186_tnx[NTINE], p186_tny[NTINE], p186_tux[NTINE], p186_tuy[NTINE];
static float p186_tof[NTINE], p186_tla[NTINE], p186_tib[NTINE], p186_tph[NTINE];
static float p186_tb2[NTINE];
static float p186_ta0[NTINE], p186_tw[NTINE];
static float p186_hue0, p186_huew, p186_bk, p186_ba, p186_drift, p186_ink[NDROP][3];
static float p186_sin[1024];
static uint32_t p186_seedc;
static int p186_ready;

static uint32_t p186_rs;
static float p186_rf(void)
{
    p186_rs ^= p186_rs << 13; p186_rs ^= p186_rs >> 17; p186_rs ^= p186_rs << 5;
    return (float)(p186_rs >> 8) * (1.0f / 16777216.0f);
}

static void p186_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static void p186_setup(uint32_t seed)
{
    int i;
    p186_rs = seed ? seed ^ 0xE0B121u : 0xE0B121u;
    p186_rf(); p186_rf();
    for (i = 0; i < NDROP; i++) {
        p186_dr2[i] = 0.030f + p186_rf() * 0.085f;
        p186_dph[i * 2] = p186_rf() * 6.2831853f;
        p186_dph[i * 2 + 1] = p186_rf() * 6.2831853f;
        p186_dhu[i] = p186_rf();
    }
    for (i = 0; i < NTINE; i++) {
        float a = p186_rf() * 3.14159265f;
        p186_tux[i] = cosf(a); p186_tuy[i] = sinf(a);
        p186_tnx[i] = -sinf(a); p186_tny[i] = cosf(a);
        p186_tof[i] = (p186_rf() - 0.5f) * 1.10f;
        p186_ta0[i] = 0.22f + p186_rf() * 0.34f;
        p186_tib[i] = 1.0f / (0.16f + p186_rf() * 0.34f);
        p186_tb2[i] = p186_tib[i] * p186_tib[i];
        p186_tph[i] = p186_rf() * 6.2831853f;
        p186_tw[i] = 0.00031f + p186_rf() * 0.00048f;
    }
    p186_hue0 = p186_rf();
    p186_huew = 0.55f + p186_rf() * 0.45f;
    p186_bk = 2.1f + p186_rf() * 3.4f;
    p186_ba = p186_rf() * 3.14159265f;
    p186_drift = (p186_rf() < 0.5f ? -1.0f : 1.0f) * (0.00055f + p186_rf() * 0.00075f);
    if (!p186_ready)
        for (i = 0; i < 1024; i++)
            p186_sin[i] = sinf((float)i * (6.2831853f / 1024.0f));
    p186_ready = 1;
    p186_seedc = seed;
}

static void p186_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p186_xmw != w) {
        free(p186_xm);
        p186_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p186_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p186_xmw = w;
    }
    jd_up_blit(&p186_up, fb, w, h, p186_img, CW, CH);
}

void pattern_186(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, bca, bsa, bph, gr[3], iph, gph;
    int x, y, i, k;
    (void)sl;
    if (!p186_ready || p186_seedc != seed) p186_setup(seed);

    for (i = 0; i < NDROP; i++) {
        p186_dcx[i] = 0.86f * sinf(t * 0.00037f + p186_dph[i * 2]);
        p186_dcy[i] = 0.66f * sinf(t * 0.00029f + p186_dph[i * 2 + 1]);
        p186_pal3(pal, p186_dhu[i] + t * 0.00013f, 0.95f, p186_ink[i]);
    }
    for (i = 0; i < NTINE; i++)
        p186_tla[i] = p186_ta0[i] * sinf(t * p186_tw[i] + p186_tph[i]);
    bca = cosf(p186_ba + t * 0.00019f) * p186_bk;
    bsa = sinf(p186_ba + t * 0.00019f) * p186_bk;
    bph = t * p186_drift;
    iph = -t * 0.0022f * (1024.0f / 6.2831853f);
    gph = t * 0.0016f * (1024.0f / 6.2831853f);
    p186_pal3(pal, 0.0f, 1.0f, gr);
    (void)gr;

    for (y = 0; y < CH; y++) {
        float y0 = ((float)y + 0.5f - CH * 0.5f) * (1.0f / 172.0f);
        unsigned char *op = p186_img + y * CW * 3;
        for (x = 0; x < CW; x++) {
            float px = ((float)x + 0.5f - CW * 0.5f) * (1.0f / 172.0f), py = y0;
            float col[3], sh, u;
            int ink = -1;
            for (k = NTINE - 1; k >= 2; k--) {
                float z = px * p186_tnx[k] + py * p186_tny[k] - p186_tof[k];
                float d = p186_tla[k] / (1.0f + z * z * p186_tb2[k]);
                px -= p186_tux[k] * d; py -= p186_tuy[k] * d;
            }
            for (k = NDROP - 1; k >= 2 && ink < 0; k--) {
                float dx = px - p186_dcx[k], dy = py - p186_dcy[k];
                float dd = dx * dx + dy * dy, s;
                if (dd <= p186_dr2[k]) { ink = k; break; }
                s = sqrtf(1.0f - p186_dr2[k] / dd);
                px = p186_dcx[k] + dx * s; py = p186_dcy[k] + dy * s;
            }
            if (ink < 0) {
                for (k = 1; k >= 0; k--) {
                    float z = px * p186_tnx[k] + py * p186_tny[k] - p186_tof[k];
                    float d = p186_tla[k] / (1.0f + z * z * p186_tb2[k]);
                    px -= p186_tux[k] * d; py -= p186_tuy[k] * d;
                }
                for (k = 1; k >= 0 && ink < 0; k--) {
                    float dx = px - p186_dcx[k], dy = py - p186_dcy[k];
                    float dd = dx * dx + dy * dy, s;
                    if (dd <= p186_dr2[k]) { ink = k; break; }
                    s = sqrtf(1.0f - p186_dr2[k] / dd);
                    px = p186_dcx[k] + dx * s; py = p186_dcy[k] + dy * s;
                }
            }
            if (ink >= 0) {
                float dx = px - p186_dcx[ink], dy = py - p186_dcy[ink];
                float q = (dx * dx + dy * dy) / p186_dr2[ink];
                sh = (1.02f - 0.30f * q * q) *
                     (0.86f + 0.14f * p186_sin[(int)(q * 2119.0f + iph) & 1023]);
                col[0] = p186_ink[ink][0] * sh;
                col[1] = p186_ink[ink][1] * sh;
                col[2] = p186_ink[ink][2] * sh;
            } else {
                uint32_t c;
                u = px * bca + py * bsa + bph;
                c = pal[((int)(u * 5215.0f)) & JD_PAL_MASK];
                sh = 0.80f + 0.20f * p186_sin[(int)(px * 277.0f - py * 375.0f + gph) & 1023];
                col[0] = (float)((c >> 16) & 255) * (1.0f / 255.0f) * sh;
                col[1] = (float)((c >> 8) & 255) * (1.0f / 255.0f) * sh;
                col[2] = (float)(c & 255) * (1.0f / 255.0f) * sh;
            }
            for (i = 0; i < 3; i++) {
                int v = (int)(col[i] * 255.0f + 0.5f);
                op[x * 3 + i] = (unsigned char)(v < 0 ? 0 : v > 255 ? 255 : v);
            }
        }
    }
    p186_blit(fb, w, h);
}
