/* 110 Firefly Sync — 200 Kuramoto oscillators finding each other.
 * Every firefly carries a phase and its own natural frequency; each frame the
 * swarm's order parameter (r, psi) is measured and every phase is pulled toward
 * it,  dtheta_i = w_i + K.r.sin(psi - theta_i)  — the standard mean-field
 * Kuramoto model, O(N) per step. The coupling K breathes slowly across the
 * critical value, so the swarm drifts in and out of coherence on its own: it
 * starts as scattered twinkling, gathers into travelling waves of light, locks
 * into one slow collective heartbeat, then falls apart again. Nobody scripts
 * that — it is the model doing it. Each fly is a soft radial glow whose flash
 * is a raised cosine to the 6th power (about a second long, never a strobe),
 * hue set by its own natural frequency, so the fast flies and the slow flies
 * read as different colour families until the swarm locks. Dots on black. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p110_up;

#define P110_LW 480
#define P110_LH 360
#define P110_N  200
#define P110_R  16                    /* glow sprite half-size */

static float p110_acc[P110_LW * P110_LH * 3];
static uint8_t p110_img[P110_LW * P110_LH * 3];
static int *p110_xm;
static int p110_xm_w;
static uint8_t p110_tone[2048];
static uint8_t p110_ramp[256][3];
static float p110_sprite[(2 * P110_R + 1) * (2 * P110_R + 1)];
static float p110_th[P110_N], p110_w[P110_N];
static float p110_bx[P110_N], p110_by[P110_N];
static float p110_ax[P110_N], p110_ay[P110_N], p110_sp[P110_N];
static uint8_t p110_hue[P110_N];
static int p110_ready;
static uint32_t p110_seen = 0xFFFFFFFFu;

static void p110_init(void)
{
    uint32_t r = 0x110F1AE5u;
    int i, j;
    for (i = 0; i < 2048; i++) {
        float v = 1.0f - expf(-(float)i * (4.4f / 2048.0f));
        p110_tone[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    for (j = -P110_R; j <= P110_R; j++)
        for (i = -P110_R; i <= P110_R; i++) {
            float d = sqrtf((float)(i * i + j * j)) * (1.0f / (float)P110_R);
            float v = 1.0f - d;
            if (v < 0.0f) v = 0.0f;
            v = v * v * (0.22f + 0.78f * v);         /* soft lantern halo */
            p110_sprite[(j + P110_R) * (2 * P110_R + 1) + (i + P110_R)] = v;
        }
    for (i = 0; i < P110_N; i++) {
        r = r * 1664525u + 1013904223u; p110_bx[i] = (float)(r >> 12 & 4095) * (P110_LW / 4096.0f);
        r = r * 1664525u + 1013904223u; p110_by[i] = (float)(r >> 12 & 4095) * (P110_LH / 4096.0f);
        r = r * 1664525u + 1013904223u; p110_th[i] = (float)(r >> 12 & 4095) * 0.0015340f;
        r = r * 1664525u + 1013904223u;
        p110_w[i] = 0.0225f + ((float)(r >> 12 & 4095) * (1.0f / 4095.0f) - 0.5f) * 0.0128f;
        r = r * 1664525u + 1013904223u; p110_ax[i] = 6.0f + (float)(r >> 20 & 255) * 0.055f;
        r = r * 1664525u + 1013904223u; p110_ay[i] = 5.0f + (float)(r >> 20 & 255) * 0.045f;
        r = r * 1664525u + 1013904223u; p110_sp[i] = (float)(r >> 12 & 4095) * 0.0015340f;
        p110_hue[i] = (uint8_t)(int)((p110_w[i] - 0.0161f) * 19900.0f);
    }
    p110_ready = 1;
}

static void p110_build_ramp(const uint32_t *pal, int base)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * 128) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p110_ramp[i][0] = p110_ramp[i-1][0];
                     p110_ramp[i][1] = p110_ramp[i-1][1];
                     p110_ramp[i][2] = p110_ramp[i-1][2]; }
            else   { p110_ramp[i][0] = p110_ramp[i][1] = p110_ramp[i][2] = 210; }
            continue;
        }
        p110_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p110_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p110_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static void p110_glow(float fx, float fy, const uint8_t *col, float amp)
{
    int cx = (int)fx, cy = (int)fy, i, j;
    int x0 = cx - P110_R, x1 = cx + P110_R;
    int y0 = cy - P110_R, y1 = cy + P110_R;
    float cr = col[0] * amp, cg = col[1] * amp, cb = col[2] * amp;
    if (x1 < 0 || y1 < 0 || x0 >= P110_LW || y0 >= P110_LH) return;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= P110_LW) x1 = P110_LW - 1;
    if (y1 >= P110_LH) y1 = P110_LH - 1;
    for (j = y0; j <= y1; j++) {
        const float *sp = p110_sprite + (j - cy + P110_R) * (2 * P110_R + 1)
                        + (x0 - cx + P110_R);
        float *p = p110_acc + (j * P110_LW + x0) * 3;
        for (i = x0; i <= x1; i++, p += 3) {
            float v = *sp++;
            p[0] += cr * v; p[1] += cg * v; p[2] += cb * v;
        }
    }
}

static void p110_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p110_xm_w != w) {
        free(p110_xm);
        p110_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p110_xm[x] = (int)(((long long)x * (P110_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p110_xm_w = w;
    }
    jd_up_blit(&p110_up, fb, w, h, p110_img, P110_LW, P110_LH);
}

void pattern_110(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float sx = 0.0f, sy = 0.0f, r, psi, K;
    int i, n3 = P110_LW * P110_LH * 3, hbase;
    (void)sl;

    if (!p110_ready) p110_init();
    if (seed != p110_seen) {                 /* new segment: re-scatter phases */
        uint32_t q = seed | 1u;
        for (i = 0; i < P110_N; i++) {
            q ^= q << 13; q ^= q >> 17; q ^= q << 5;
            p110_th[i] += (float)(q >> 12 & 4095) * 0.0004f;
        }
        p110_seen = seed;
    }
    hbase = (int)(t * 1.1f) + (int)(seed & 32767);
    p110_build_ramp(pal, hbase);

    /* order parameter of the swarm */
    for (i = 0; i < P110_N; i++) {
        sx += cosf(p110_th[i]); sy += sinf(p110_th[i]);
    }
    sx *= 1.0f / (float)P110_N; sy *= 1.0f / (float)P110_N;
    r = sqrtf(sx * sx + sy * sy);
    psi = atan2f(sy, sx);
    K = 0.030f + 0.028f * sinf(t * 0.00047f + (float)(seed & 1023) * 0.0061f);

    memset(p110_acc, 0, sizeof p110_acc);
    for (i = 0; i < P110_N; i++) {
        float th = p110_th[i] + p110_w[i] + K * r * sinf(psi - p110_th[i]);
        float c = cosf(th);
        float b = (0.5f + 0.5f * c);
        float b3 = b * b * (0.30f + 0.70f * b);
        float px, py;
        p110_th[i] = th > 6.2831853f ? th - 6.2831853f : th;
        b3 = b3 * b3;                                     /* flash = b^6 */
        px = p110_bx[i] + p110_ax[i] * sinf(t * 0.0021f + p110_sp[i] * 400.0f);
        py = p110_by[i] + p110_ay[i] * sinf(t * 0.0017f + p110_sp[i] * 730.0f);
        p110_glow(px, py, p110_ramp[(hbase / 14 + p110_hue[i]) & 255],
                  0.17f + 3.00f * b3);
    }
    /* faint collective bloom when the swarm is coherent */
    if (r > 0.35f) {
        float g = (r - 0.35f) * 0.9f;
        float pulse = 0.5f + 0.5f * cosf(psi);
        float add = g * pulse * pulse * 3.0f;
        const uint8_t *cp = p110_ramp[(hbase / 14 + 128) & 255];
        for (i = 0; i < n3; i += 3) {
            p110_acc[i + 0] += cp[0] * add * 0.0035f;
            p110_acc[i + 1] += cp[1] * add * 0.0035f;
            p110_acc[i + 2] += cp[2] * add * 0.0035f;
        }
    }

    for (i = 0; i < n3; i++) {
        int ti = (int)(p110_acc[i] * 1.5f);
        if (ti > 2047) ti = 2047;
        p110_img[i] = p110_tone[ti];
    }
    p110_blit(fb, w, h);
}
