/* 103 Dragon Unfold — the Heighway dragon caught mid-fold.
 * Take a strip of paper, fold it in half thirteen times the same way, open every
 * crease to the same angle and you get this curve. The crease directions are the
 * paperfolding sequence, turn(i) = ((i & -i) << 1) & i, computed once for 8192
 * segments; the only thing that animates is the angle every crease opens to,
 * which glides between 42 and 90 degrees. At 90 the curve is the familiar
 * dragon fractal that tiles the plane; below that it relaxes into scrolls,
 * ribbons and spirals nobody has a name for, and it passes through all of them
 * continuously — one line, never cut, never redrawn. Two copies rotated a half
 * turn about the shared origin close it into a rosette (the twindragon). The
 * frame is auto-fitted with a lagged scale so the figure breathes instead of
 * jumping, and hue runs along arclength so you can see the fold order. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p103_up;

#define P103_LW 480
#define P103_LH 360
#define P103_N  8192

static float p103_acc[P103_LW * P103_LH * 3];
static uint8_t p103_img[P103_LW * P103_LH * 3];
static int *p103_xm;
static int p103_xm_w;
static uint8_t p103_tone[2048];
static uint8_t p103_ramp[256][3];
static uint8_t p103_turn[P103_N];
static float p103_px[P103_N + 1], p103_py[P103_N + 1];
static float p103_fs = 1.0f, p103_fx, p103_fy;
static float p103_gain = 3.0f;
static int p103_ready;

static void p103_init(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 1.0f - expf(-(float)i * (4.2f / 2048.0f));
        p103_tone[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    for (i = 1; i < P103_N; i++)
        p103_turn[i] = (uint8_t)((((i & -i) << 1) & i) ? 1 : 0);
    p103_ready = 1;
}

static void p103_build_ramp(const uint32_t *pal, int base)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * 128) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p103_ramp[i][0] = p103_ramp[i-1][0];
                     p103_ramp[i][1] = p103_ramp[i-1][1];
                     p103_ramp[i][2] = p103_ramp[i-1][2]; }
            else   { p103_ramp[i][0] = p103_ramp[i][1] = p103_ramp[i][2] = 210; }
            continue;
        }
        p103_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p103_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p103_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static void p103_dot(float x, float y, const uint8_t *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, sk, *p;
    float r = c[0] * wgt, g = c[1] * wgt, b = c[2] * wgt;
    if ((unsigned)(xi - 1) >= P103_LW - 3 || (unsigned)(yi - 1) >= P103_LH - 3) return;
    fx = x - (float)xi; fy = y - (float)yi;
    sk = 0.42f;
    p = p103_acc + (yi * P103_LW + xi) * 3;
    {
        float w00 = (1.0f - fx) * (1.0f - fy), w10 = fx * (1.0f - fy);
        float w01 = (1.0f - fx) * fy, w11 = fx * fy;
        p[0] += r * w00; p[1] += g * w00; p[2] += b * w00;
        p[3] += r * w10; p[4] += g * w10; p[5] += b * w10;
        p[-3] += r * sk; p[-2] += g * sk; p[-1] += b * sk;
        p[6] += r * sk; p[7] += g * sk; p[8] += b * sk;
        p -= P103_LW * 3;
        p[0] += r * sk; p[1] += g * sk; p[2] += b * sk;
        p += P103_LW * 6;
        p[0] += r * w01; p[1] += g * w01; p[2] += b * w01;
        p[3] += r * w11; p[4] += g * w11; p[5] += b * w11;
        p += P103_LW * 3;
        p[0] += r * sk; p[1] += g * sk; p[2] += b * sk;
    }
}

static void p103_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p103_xm_w != w) {
        free(p103_xm);
        p103_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p103_xm[x] = (int)(((long long)x * (P103_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p103_xm_w = w;
    }
    jd_up_blit(&p103_up, fb, w, h, p103_img, P103_LW, P103_LH);
}

void pattern_103(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float th, ct, st, dx, dy, x, y;
    float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
    float sx, sy, sc, rc, rs, spin;
    int i, n3 = P103_LW * P103_LH * 3, hbase;
    (void)sl;

    if (!p103_ready) p103_init();
    hbase = (int)(t * 1.6f) + (int)(seed & 32767);
    p103_build_ramp(pal, hbase);

    {
        float ph = (float)(seed & 4095) * 0.00153f;
        th = (0.800f + 0.200f * sinf(t * 0.00163f + ph)) * 1.5707963f;
        spin = t * 0.00042f + ph;
    }
    ct = cosf(th); st = sinf(th);
    rc = cosf(spin); rs = sinf(spin);

    /* walk the fold sequence, tracking the bounding box as we go */
    dx = 1.0f; dy = 0.0f; x = 0.0f; y = 0.0f;
    p103_px[0] = 0.0f; p103_py[0] = 0.0f;
    for (i = 1; i <= P103_N; i++) {
        x += dx; y += dy;
        p103_px[i] = x; p103_py[i] = y;
        if (x < x0) x0 = x; if (x > x1) x1 = x;
        if (y < y0) y0 = y; if (y > y1) y1 = y;
        if (i < P103_N) {                    /* open crease i to angle th */
            float ndx, ndy;
            if (p103_turn[i]) { ndx = dx * ct + dy * st; ndy = -dx * st + dy * ct; }
            else              { ndx = dx * ct - dy * st; ndy =  dx * st + dy * ct; }
            dx = ndx; dy = ndy;
        }
    }
    if (x0 > 0.0f) x0 = 0.0f; if (x1 < 0.0f) x1 = 0.0f;
    if (y0 > 0.0f) y0 = 0.0f; if (y1 < 0.0f) y1 = 0.0f;

    /* the twindragon is point-symmetric about the origin, so fit the union */
    {
        float ex = (x1 > -x0 ? x1 : -x0), ey = (y1 > -y0 ? y1 : -y0);
        float wx, wy, want;
        /* the union with its half-turn twin is symmetric about the origin, so
         * the half-extents are all the fit needs */
        if (ex < 1.0f) ex = 1.0f;
        if (ey < 1.0f) ey = 1.0f;
        wx = (P103_LW * 0.465f) / ex;
        wy = (P103_LH * 0.465f) / ey;
        want = wx < wy ? wx : wy;
        p103_fs += (want - p103_fs) * 0.045f;        /* lagged fit: no jumps */
    }
    sc = p103_fs;
    sx = P103_LW * 0.5f; sy = P103_LH * 0.5f;
    p103_fx = sx; p103_fy = sy;

    memset(p103_acc, 0, sizeof p103_acc);
    {
        /* every segment is the same length (sc px), so one substep count keeps
         * the stroke solid and its brightness independent of the current fit */
        int nsub = (int)(sc * 1.35f) + 1;
        float inv = 1.0f / (float)nsub;
        float wgt = 0.085f * inv;
        for (i = 0; i < P103_N; i++) {
            float ax = p103_px[i] * sc, ay = p103_py[i] * sc;
            float gx = (p103_px[i + 1] - p103_px[i]) * sc * inv;
            float gy = (p103_py[i + 1] - p103_py[i]) * sc * inv;
            const uint8_t *cp = p103_ramp[(hbase / 12 + (i >> 5)) & 255];
            int k;
            for (k = 0; k < nsub; k++) {
                float px = ax + gx * (float)k, py = ay + gy * (float)k;
                float bx = px * rc - py * rs, by = px * rs + py * rc;
                p103_dot(sx + bx, sy + by, cp, wgt);
                p103_dot(sx - bx, sy - by, cp, wgt); /* the twin */
            }
        }
    }

    /* Auto-exposure. As the creases close the curve folds onto itself and the
     * ink per pixel swings by two orders of magnitude; a fixed gain would make
     * the unfolded state blow out and the folded state go black. The mean of
     * the accumulator drives a lagged gain instead, so exposure tracks the
     * fold without any visible pumping. */
    {
        double sum = 0.0;
        float g = p103_gain;
        for (i = 0; i < n3; i++) {
            float v = p103_acc[i];
            int ti = (int)(v * g);
            sum += v;
            if (ti > 2047) ti = 2047;
            p103_img[i] = p103_tone[ti];
        }
        {
            float mean = (float)(sum / (double)n3);
            float want = 22.0f / (mean > 1e-4f ? mean : 1e-4f);
            if (want > 90.0f) want = 90.0f;
            if (want < 0.35f) want = 0.35f;
            p103_gain += (want - p103_gain) * 0.05f;
        }
    }
    p103_blit(fb, w, h);
}
