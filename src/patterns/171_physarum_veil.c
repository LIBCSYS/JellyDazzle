/* 171 Physarum Veil — a slime-mould colony writing its own transport network.
 * 22000 agents each sense the pheromone field ahead-left / ahead / ahead-right,
 * steer toward the strongest, deposit, and move 0.6 px per frame. The field is
 * blurred and decayed every frame, so the colony first fogs the plate, then
 * condenses into veins, then prunes the veins into a sparse lace of hubs and
 * filaments. Nothing is drawn but the pheromone itself: bright cords, dark
 * cells between them, black everywhere the mould never went — an overlay that
 * reads as luminous mycelial handwriting. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p171_up;

#define P171_W 480
#define P171_H 360
#define P171_NA 22000
#define P171_TAU 6.28318530717958647692f

static float p171_acc[P171_W * P171_H * 3];
static float p171_tr[P171_W * P171_H];
static float p171_tr2[P171_W * P171_H];
static unsigned char p171_img[P171_W * P171_H * 3];
static unsigned char p171_tone[1024];
static int *p171_xm;
static int p171_xmw;
static int p171_ready;
static uint32_t p171_seedc;
static int p171_armed;

static float p171_ax[P171_NA], p171_ay[P171_NA];
static float p171_dx[P171_NA], p171_dy[P171_NA];
static float p171_col[64][3];
static float p171_hue0, p171_huew, p171_sa, p171_sd, p171_turn, p171_dec;

static uint32_t p171_rs;
static float p171_rf(void)
{
    p171_rs ^= p171_rs << 13; p171_rs ^= p171_rs >> 17; p171_rs ^= p171_rs << 5;
    return (float)(p171_rs >> 8) * (1.0f / 16777216.0f);
}

static void p171_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 64; i++) {
        float hue = p171_hue0 + p171_huew * ((float)i / 63.0f);
        float r, g, b, mx, lift;
        uint32_t p;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        lift = (float)i / 63.0f;
        lift = lift * lift * lift * lift;        /* only the hottest go pale */
        p171_col[i][0] = (0.06f + 0.94f * r / mx) * (1.0f - lift) + lift * 1.0f;
        p171_col[i][1] = (0.06f + 0.94f * g / mx) * (1.0f - lift) + lift * 0.94f;
        p171_col[i][2] = (0.06f + 0.94f * b / mx) * (1.0f - lift) + lift * 0.88f;
    }
}

static void p171_setup(uint32_t seed)
{
    int i;
    p171_rs = seed ? seed ^ 0x51AEB0D1u : 0x51AEB0D1u;
    p171_rf(); p171_rf();
    p171_hue0 = p171_rf();
    p171_huew = 0.05f + p171_rf() * 0.30f;
    p171_sa   = 0.30f + p171_rf() * 0.24f;        /* sensor half-angle       */
    p171_sd   = 3.0f + p171_rf() * 3.2f;          /* sensor distance         */
    p171_turn = 0.55f + p171_rf() * 0.45f;
    p171_dec  = 0.905f + p171_rf() * 0.045f;
    for (i = 0; i < P171_NA; i++) {
        float a = p171_rf() * P171_TAU;
        float r = sqrtf(p171_rf()) * (P171_H * 0.40f);
        float b = p171_rf() * P171_TAU;
        p171_ax[i] = P171_W * 0.5f + r * cosf(a);
        p171_ay[i] = P171_H * 0.5f + r * sinf(a);
        p171_dx[i] = cosf(b); p171_dy[i] = sinf(b);
    }
    memset(p171_tr, 0, sizeof p171_tr);
    if (!p171_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (4.2f / 1024.0f)));
            p171_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p171_ready = 1;
    }
    p171_seedc = seed;
    p171_armed = 1;
}

static float p171_sense(float x, float y)
{
    int xi, yi;
    if (x < 0.0f) x += P171_W; else if (x >= P171_W) x -= P171_W;
    if (y < 0.0f) y += P171_H; else if (y >= P171_H) y -= P171_H;
    xi = (int)x; yi = (int)y;
    if ((unsigned)xi >= P171_W) xi = 0;
    if ((unsigned)yi >= P171_H) yi = 0;
    return p171_tr[yi * P171_W + xi];
}

static void p171_deposit(float x, float y, float w)
{
    int xi = (int)x, yi = (int)y, x1, y1;
    float fx, fy;
    float *t = p171_tr;
    if ((unsigned)xi >= P171_W || (unsigned)yi >= P171_H) return;
    fx = x - (float)xi; fy = y - (float)yi;
    x1 = xi + 1 < P171_W ? xi + 1 : 0;
    y1 = yi + 1 < P171_H ? yi + 1 : 0;
    t[yi * P171_W + xi] += (1.0f - fx) * (1.0f - fy) * w;
    t[yi * P171_W + x1] += fx * (1.0f - fy) * w;
    t[y1 * P171_W + xi] += (1.0f - fx) * fy * w;
    t[y1 * P171_W + x1] += fx * fy * w;
}

static void p171_diffuse(void)
{
    int y, x;
    float d = p171_dec;
    for (y = 0; y < P171_H; y++) {
        int ym = (y == 0 ? P171_H - 1 : y - 1) * P171_W;
        int yp = (y == P171_H - 1 ? 0 : y + 1) * P171_W;
        int yo = y * P171_W;
        for (x = 0; x < P171_W; x++) {
            int xm = x == 0 ? P171_W - 1 : x - 1;
            int xp = x == P171_W - 1 ? 0 : x + 1;
            float v = p171_tr[yo + x] * 0.60f
                    + 0.10f * (p171_tr[yo + xm] + p171_tr[yo + xp]
                             + p171_tr[ym + x] + p171_tr[yp + x]);
            p171_tr2[yo + x] = v * d;
        }
    }
    memcpy(p171_tr, p171_tr2, sizeof p171_tr);
}

static void p171_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P171_W * P171_H * 3; i++) {
        int ti = (int)(p171_acc[i] * 256.0f);
        p171_img[i] = p171_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p171_xmw != w) {
        free(p171_xm);
        p171_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p171_xm[x] = (int)(((long long)x * (P171_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p171_xmw = w;
    }
    jd_up_blit(&p171_up, fb, w, h, p171_img, P171_W, P171_H);
}

void pattern_171(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    float spd, sd, csa, ssa, ctn, stn;
    if (!p171_armed || p171_seedc != seed || sl == 0) p171_setup(seed);
    p171_hues(pal);

    spd = 0.45f;
    sd  = p171_sd;
    csa = cosf(p171_sa); ssa = sinf(p171_sa);
    ctn = cosf(p171_turn); stn = sinf(p171_turn);

    for (i = 0; i < P171_NA; i++) {
        float x = p171_ax[i], y = p171_ay[i];
        float dx = p171_dx[i], dy = p171_dy[i];
        float lx = dx * csa - dy * ssa, ly = dx * ssa + dy * csa;
        float rx = dx * csa + dy * ssa, ry = -dx * ssa + dy * csa;
        float F = p171_sense(x + dx * sd, y + dy * sd);
        float L = p171_sense(x + lx * sd, y + ly * sd);
        float R = p171_sense(x + rx * sd, y + ry * sd);
        float nx, ny;
        if (F >= L && F >= R) {
            nx = dx; ny = dy;
        } else if (L > R) {
            nx = dx * ctn - dy * stn; ny = dx * stn + dy * ctn;
        } else if (R > L) {
            nx = dx * ctn + dy * stn; ny = -dx * stn + dy * ctn;
        } else if (p171_rf() < 0.5f) {
            nx = dx * ctn - dy * stn; ny = dx * stn + dy * ctn;
        } else {
            nx = dx * ctn + dy * stn; ny = -dx * stn + dy * ctn;
        }
        /* tiny stochastic wobble, kept unit by first-order renormalisation */
        {
            float j = (p171_rf() - 0.5f) * 0.10f;
            float ax2 = nx - ny * j, ay2 = ny + nx * j;
            float q = 1.5f - 0.5f * (ax2 * ax2 + ay2 * ay2);
            nx = ax2 * q; ny = ay2 * q;
        }
        x += nx * spd; y += ny * spd;
        if (x < 0.0f) x += P171_W; else if (x >= P171_W) x -= P171_W;
        if (y < 0.0f) y += P171_H; else if (y >= P171_H) y -= P171_H;
        p171_ax[i] = x; p171_ay[i] = y; p171_dx[i] = nx; p171_dy[i] = ny;
        p171_deposit(x, y, 0.22f);
    }
    /* a trickle of scouts is reseeded every frame: without it the colony
       coarsens into three cords and stops being a network */
    for (i = 0; i < 70; i++) {
        int k = (int)(p171_rf() * (float)P171_NA);
        float b = p171_rf() * P171_TAU;
        if (k >= P171_NA) k = P171_NA - 1;
        p171_ax[k] = p171_rf() * (float)P171_W;
        p171_ay[k] = p171_rf() * (float)P171_H;
        p171_dx[k] = cosf(b); p171_dy[k] = sinf(b);
    }
    p171_diffuse();

    {
        float gain = 0.42f + 0.06f * sinf((float)frame * 0.0013f);
        for (i = 0; i < P171_W * P171_H; i++) {
            float v = p171_tr[i] * gain;
            float inten;
            const float *c;
            int k;
            if (v > 2.1f) v = 2.1f;
            inten = v;
            k = (int)(63.0f * (1.0f - expf(-v * 0.95f)));
            if (k > 63) k = 63;
            c = p171_col[k];
            p171_acc[i * 3 + 0] = c[0] * inten;
            p171_acc[i * 3 + 1] = c[1] * inten;
            p171_acc[i * 3 + 2] = c[2] * inten;
        }
    }
    p171_blit(fb, w, h);
}
