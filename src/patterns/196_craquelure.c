/* 196 Craquelure — a crack network writing itself into a black glaze.
 * Forty crack tips creep forward about a third of a pixel per frame, wandering
 * by a few hundredths of a radian and occasionally throwing a branch off at a
 * right angle. Each tip stamps its own trail into an occupancy grid a little
 * way behind itself, and dies the moment it runs into anybody else's trail —
 * which is the one rule that turns random walks into real craquelure, because
 * cracks terminate on cracks and never cross. Dead tips are replaced, and the
 * whole canvas loses a fraction of a percent of its light per frame, so old
 * lattices sink away while new ones grow over them. Almost nothing changes
 * between two frames: this is the quietest thing in the set. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p196_up;

#define P196_W 480
#define P196_H 360
#define P196_TAU 6.28318530717958647692f

static float p196_acc[P196_W * P196_H * 3];
static unsigned char p196_img[P196_W * P196_H * 3];
static unsigned char p196_tone[1024];
static int *p196_xm;
static int p196_xmw;
static int p196_tone_ok;
static uint32_t p196_rs = 1u;

static float p196_rf(void)
{
    p196_rs ^= p196_rs << 13; p196_rs ^= p196_rs >> 17; p196_rs ^= p196_rs << 5;
    return (float)(p196_rs >> 8) * (1.0f / 16777216.0f);
}

static void p196_tone_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (8.50f / 1024.0f)));
        p196_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
    }
    p196_tone_ok = 1;
}

/* palette sample, brightness-normalised so dark ramp zones still read as light */
static void p196_col(const uint32_t *pal, float hue, float lift, float *out)
{
    uint32_t p; float r, g, b, mx;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
    mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    out[0] = lift + (1.0f - lift) * r / mx;
    out[1] = lift + (1.0f - lift) * g / mx;
    out[2] = lift + (1.0f - lift) * b / mx;
}


static void p196_splat(float x, float y, const float *c, float w)
{
    int xi, yi; float fx, fy, w0, w1; float *p;
    if (!(x >= 0.0f) || !(y >= 0.0f)) return;
    xi = (int)x; yi = (int)y;
    if (xi >= P196_W - 1 || yi >= P196_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p196_acc + (yi * P196_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P196_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}


/* --- glaze and pre-roll (v2.1) -------------------------------------------
 * Measured before: luma 0.01 at sl==0, 0.35 at half a second, 2.99 at five
 * seconds, and only 12.5 by the end of a 34-second segment — the accumulator
 * cleared to absolute zero, and forty tips creeping a third of a pixel a frame
 * take most of a minute to write a network worth looking at.  The header
 * called the ground "a black glaze", but a glaze is a fired surface with a
 * colour and a sheen, not a hole: it is now painted as one, dim and vignetted,
 * with the cracks tone-mapped ON TOP.  P196_PRE frames of growth are replayed
 * at reset so the segment opens on an established lattice. */
#define P196_PRE  1100        /* frames of crack growth replayed at reset  */
#define P196_GB     26.0f     /* glaze brightness                          */
static unsigned char p196_vig[P196_W * P196_H];
static unsigned char p196_gt[256][3];   /* glaze by vignette level, per frame */
static int   p196_vready = 0, p196_need_pre = 0;

static void p196_mkvig(void)
{
    int x, y;
    for (y = 0; y < P196_H; y++)
        for (x = 0; x < P196_W; x++) {
            float dx = ((float)x - P196_W * 0.5f) / (P196_W * 0.5f);
            float dy = ((float)y - P196_H * 0.5f) / (P196_H * 0.5f);
            float v = 1.0f - 0.40f * (dx * dx + dy * dy);
            if (v < 0.12f) v = 0.12f;
            p196_vig[y * P196_W + x] = (unsigned char)lrintf(v * 255.0f);
        }
    p196_vready = 1;
}

static void p196_blit(uint32_t *fb, int w, int h)
{
    int x, y, c, i;
    for (y = 0, i = 0; y < P196_H; y++) {
        const unsigned char *vg = p196_vig + y * P196_W;
        for (x = 0; x < P196_W; x++) {
            const unsigned char *g = p196_gt[vg[x]];
            for (c = 0; c < 3; c++, i++) {
                int ti = (int)(p196_acc[i] * 256.0f);
                int v = p196_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti] + g[c];
                p196_img[i] = (unsigned char)(v > 255 ? 255 : v);
            }
        }
    }
    if (p196_xmw != w) {
        free(p196_xm);
        p196_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p196_xm[x] = (int)(((long long)x * (P196_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p196_xmw = w;
    }
    jd_up_blit(&p196_up, fb, w, h, p196_img, P196_W, P196_H);
}

#define P196_MT 128
#define P196_ACT 26

static uint32_t p196_seedc = 0xFFFFFFFFu;
static float p196_h0, p196_hw;
static float p196_tx[P196_MT], p196_ty[P196_MT], p196_ta[P196_MT];
static float p196_tw[P196_MT];
static short p196_thu[P196_MT];
static int p196_tlive[P196_MT], p196_tage[P196_MT];
static float p196_lag[P196_MT][8][2];
static unsigned char p196_occ[P196_W * P196_H];
static float p196_hue[32][3];
static int p196_lastsl = -99999;

static void p196_seedtip(int i)
{
    int try_;
    p196_tx[i] = 12.0f + p196_rf() * (float)(P196_W - 24);
    p196_ty[i] = 12.0f + p196_rf() * (float)(P196_H - 24);
    for (try_ = 0; try_ < 24; try_++) {
        float qx = 12.0f + p196_rf() * (float)(P196_W - 24);
        float qy = 12.0f + p196_rf() * (float)(P196_H - 24);
        if (p196_occ[(int)qy * P196_W + (int)qx]) { p196_tx[i] = qx; p196_ty[i] = qy; break; }
    }
    p196_ta[i] = p196_rf() * P196_TAU;
    p196_tw[i] = 0.55f + p196_rf() * 0.65f;
    p196_thu[i] = (short)(p196_rf() * 31.99f);
    p196_tlive[i] = 1;
    p196_tage[i] = 0;
    {   int k;
        for (k = 0; k < 8; k++) { p196_lag[i][k][0] = p196_tx[i]; p196_lag[i][k][1] = p196_ty[i]; } }
}

static void p196_reset(uint32_t seed)
{
    int i;
    p196_rs = seed ? seed * 2654435761u + 0x2545F491u : 0x196u;
    p196_rf(); p196_rf();
    p196_h0 = p196_rf();
    p196_hw = 0.05f + p196_rf() * 0.50f;
    memset(p196_acc, 0, sizeof p196_acc);
    memset(p196_occ, 0, sizeof p196_occ);
    for (i = 0; i < P196_MT; i++) p196_tlive[i] = 0;
    for (i = 0; i < P196_ACT; i++) p196_seedtip(i);
    p196_seedc = seed;
    p196_need_pre = 1;
    if (!p196_tone_ok) p196_tone_init();
}

static void p196_mark(float x, float y)
{
    int xi = (int)x, yi = (int)y, dx, dy;
    for (dy = -1; dy <= 1; dy++)
        for (dx = -1; dx <= 1; dx++) {
            int px = xi + dx, py = yi + dy;
            if ((unsigned)px < (unsigned)P196_W && (unsigned)py < (unsigned)P196_H)
                p196_occ[py * P196_W + px] = 1;
        }
}

static void p196_soft(float x, float y, const float *c, float w)
{
    p196_splat(x, y, c, w);
    p196_splat(x + 1.0f, y, c, w * 0.26f);
    p196_splat(x - 1.0f, y, c, w * 0.26f);
    p196_splat(x, y + 1.0f, c, w * 0.26f);
    p196_splat(x, y - 1.0f, c, w * 0.26f);
}

/* One frame of crack growth.  `wmul` scales every deposit: the canvas decay is
 * a single uniform factor per frame, so replaying N frames of history with
 * deposit weight decay^(N-j) lands on exactly the canvas those N frames would
 * have produced — the same trick pattern_031 uses to prime its accumulator,
 * and it costs one multiply instead of N passes over 518k floats. */
static void p196_advance(float wmul)
{
    int i, k, nlive = 0;
    for (i = 0; i < P196_MT; i++) {
        float nx, ny, ax, ay;
        if (!p196_tlive[i]) continue;
        p196_ta[i] += (p196_rf() - 0.5f) * 0.042f;
        nx = p196_tx[i] + cosf(p196_ta[i]) * 0.62f;
        ny = p196_ty[i] + sinf(p196_ta[i]) * 0.62f;
        ax = nx + cosf(p196_ta[i]) * 3.2f;
        ay = ny + sinf(p196_ta[i]) * 3.2f;
        if (nx < 2.0f || ny < 2.0f || nx > (float)P196_W - 3.0f || ny > (float)P196_H - 3.0f) {
            p196_tlive[i] = 0; continue;
        }
        if ((unsigned)(int)ax < (unsigned)P196_W && (unsigned)(int)ay < (unsigned)P196_H
            && p196_occ[(int)ay * P196_W + (int)ax] && p196_tage[i] > 40) {
            p196_soft(nx, ny, p196_hue[p196_thu[i]], p196_tw[i] * 0.34f * wmul);
            p196_tlive[i] = 0; continue;
        }
        p196_soft(nx, ny, p196_hue[p196_thu[i]], p196_tw[i] * 0.115f * wmul);
        p196_tx[i] = nx; p196_ty[i] = ny; p196_tage[i]++;
        /* mark the trail eight steps back so a tip never trips over itself */
        p196_mark(p196_lag[i][7][0], p196_lag[i][7][1]);
        for (k = 7; k > 0; k--) {
            p196_lag[i][k][0] = p196_lag[i][k - 1][0];
            p196_lag[i][k][1] = p196_lag[i][k - 1][1];
        }
        p196_lag[i][0][0] = nx; p196_lag[i][0][1] = ny;
        if (p196_tage[i] > 60 && p196_rf() < 0.0090f) {
            for (k = 0; k < P196_MT; k++)
                if (!p196_tlive[k]) {
                    p196_seedtip(k);
                    p196_tx[k] = nx; p196_ty[k] = ny;
                    p196_ta[k] = p196_ta[i] + (p196_rf() < 0.5f ? 1.48f : -1.48f);
                    p196_thu[k] = p196_thu[i];
                    p196_tw[k] = p196_tw[i] * 0.82f;
                    { int q; for (q = 0; q < 8; q++) {
                        p196_lag[k][q][0] = nx; p196_lag[k][q][1] = ny; } }
                    break;
                }
        }
        nlive++;
    }
    for (i = 0; i < P196_MT && nlive < P196_ACT; i++)
        if (!p196_tlive[i]) { p196_seedtip(i); nlive++; }
}

void pattern_196(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    (void)frame;
    if (!p196_vready) p196_mkvig();
    if (p196_seedc != seed || sl == 0 || sl != p196_lastsl + 1) p196_reset(seed);
    p196_lastsl = sl;
    for (i = 0; i < 32; i++)
        p196_col(pal, p196_h0 + p196_hw * ((float)i / 31.0f), 0.16f, p196_hue[i]);
    /* Glaze: a deep, desaturated fired surface offset from the crack hues, as
     * a 256-entry table keyed by vignette level so the blit stays integer. */
    {
        float gc[3];
        p196_col(pal, p196_h0 + 0.44f, 0.30f, gc);
        for (i = 0; i < 256; i++) {
            float ge = P196_GB * ((float)i * (1.0f / 255.0f));
            p196_gt[i][0] = (unsigned char)(int)(gc[0] * ge);
            p196_gt[i][1] = (unsigned char)(int)(gc[1] * ge);
            p196_gt[i][2] = (unsigned char)(int)(gc[2] * ge);
        }
    }

    /* The pre-roll has to happen here rather than in p196_reset because the
     * deposits are palette-coloured and reset does not see the palette. */
    if (p196_need_pre) {
        for (i = 0; i < P196_PRE; i++)
            p196_advance(powf(0.99930f, (float)(P196_PRE - i)));
        p196_need_pre = 0;
    }

    for (i = 0; i < P196_W * P196_H * 3; i++) p196_acc[i] *= 0.99930f;
    p196_advance(1.0f);
    p196_blit(fb, w, h);
}
