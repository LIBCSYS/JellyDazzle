/* 117 Cloth Lattice — a real membrane simulation seen as a wireframe. A 34x26
 * grid runs the discrete wave equation u'' = c^2 * laplacian(u) with the border
 * pinned and a whisper of damping; three slow driver points breathe energy in,
 * so standing-wave modes rise, interfere and decay of their own accord — the
 * shape is never scripted. The lattice is yawed and pitched in 3D and
 * perspective-projected, edges drawn as glowing wire with depth-cued brightness
 * and height-cued hue. Wire on black, deliberately transparent: an overlay that
 * gives a stack its geometry. The seed drives colour only: the membrane
 * state is persistent, so a segment change never cuts the shape.
 */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
static jd_up p117_up;

#define P117_LW 640
#define P117_LH 480
#define P117_NX 34
#define P117_NY 26
#define P117_N  (P117_NX * P117_NY)

static float p117_u[P117_N], p117_uo[P117_N];
static int p117_init;
static float p117_acc[P117_LW * P117_LH * 3];
static float p117_tmp[P117_LW * P117_LH * 3];
static unsigned char p117_img[P117_LW * P117_LH * 3];
static float p117_ramp[256][3];
static float p117_px[P117_N], p117_py[P117_N], p117_pd[P117_N];

static void p117_ramp_build(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i * 128) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 8.0f) mx = 8.0f;
        p117_ramp[i][0] = r / mx; p117_ramp[i][1] = g / mx; p117_ramp[i][2] = b / mx;
    }
}

static void p117_splat(float x, float y, float r, float g, float b, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1, w2, w3; float *p;
    if (xi < 0 || yi < 0 || xi >= P117_LW - 1 || yi >= P117_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    w2 = (1.0f - fx) * fy * w;         w3 = fx * fy * w;
    p = p117_acc + ((size_t)yi * P117_LW + xi) * 3;
    p[0] += r * w0; p[1] += g * w0; p[2] += b * w0;
    p[3] += r * w1; p[4] += g * w1; p[5] += b * w1;
    p += P117_LW * 3;
    p[0] += r * w2; p[1] += g * w2; p[2] += b * w2;
    p[3] += r * w3; p[4] += g * w3; p[5] += b * w3;
}

static void p117_wire(int a, int b, int hbase, float t)
{
    float x0 = p117_px[a], y0 = p117_py[a], x1 = p117_px[b], y1 = p117_py[b];
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    float d = 0.5f * (p117_pd[a] + p117_pd[b]);
    float bright = 0.30f + 1.15f / (1.0f + 8.0f * d * d);
    float hgt = 0.5f * (p117_u[a] + p117_u[b]);
    const float *col;
    int n, i, hi;
    if (len < 0.01f || len > 420.0f) return;
    hi = (hbase + (int)(hgt * 900.0f) + (int)(d * 40.0f)) & 255;
    col = p117_ramp[hi];
    bright *= 0.55f + 0.45f * (0.5f + 0.5f * sinf(hgt * 26.0f - 0.02f * t));
    n = (int)(len * 1.25f) + 2;
    if (n > 320) n = 320;
    {
        float ix = dx / (float)n, iy = dy / (float)n, w = bright * 0.46f;
        for (i = 0; i <= n; i++)
            p117_splat(x0 + ix * (float)i, y0 + iy * (float)i,
                       col[0], col[1], col[2], w);
    }
}

static void p117_blit(uint32_t *fb, int w, int h)
{
    int i, x, y, c, n = P117_LW * P117_LH * 3;
    for (y = 0; y < P117_LH; y++) {
        const float *s = p117_acc + (size_t)y * P117_LW * 3;
        float *d = p117_tmp + (size_t)y * P117_LW * 3;
        for (x = 0; x < P117_LW; x++) {
            int xm1 = x > 0 ? x - 1 : 0, xp1 = x < P117_LW - 1 ? x + 1 : P117_LW - 1;
            for (c = 0; c < 3; c++)
                d[x * 3 + c] = 0.27f * (s[xm1 * 3 + c] + s[xp1 * 3 + c]) +
                               0.46f * s[x * 3 + c];
        }
    }
    for (x = 0; x < P117_LW; x++)
        for (y = 0; y < P117_LH; y++) {
            int ym1 = y > 0 ? y - 1 : 0, yp1 = y < P117_LH - 1 ? y + 1 : P117_LH - 1;
            for (c = 0; c < 3; c++) {
                size_t o = (size_t)x * 3 + (size_t)c;
                float v = 0.27f * (p117_tmp[(size_t)ym1 * P117_LW * 3 + o] +
                                   p117_tmp[(size_t)yp1 * P117_LW * 3 + o]) +
                          0.46f * p117_tmp[(size_t)y * P117_LW * 3 + o];
                p117_acc[(size_t)y * P117_LW * 3 + o] += 1.05f * v;
            }
        }
    for (i = 0; i < n; i++) {
        float cc = p117_acc[i], v = 255.0f * cc / (0.9f + cc);
        p117_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    jd_up_blit(&p117_up, fb, w, h, p117_img, P117_LW, P117_LH);
}

void pattern_117(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sp = (float)(seed & 1023) * 0.006136f;
    float yaw, cy_, sy_, pitch, cp, spn, hs, scale;
    int i, j, o, hbase;
    (void)sl;

    p117_ramp_build(pal);
    if (!p117_init) {
        memset(p117_u, 0, sizeof p117_u);
        memset(p117_uo, 0, sizeof p117_uo);
        p117_init = 1;
    }

    /* --- membrane step: u'' = c^2 laplacian(u), pinned border, light damping */
    {
        float c2 = 0.028f, dmp = 0.9994f;
        static float un[P117_N], vv[P117_N];
        for (j = 1; j < P117_NY - 1; j++)
            for (i = 1; i < P117_NX - 1; i++) {
                o = j * P117_NX + i;
                un[o] = (2.0f * p117_u[o] - p117_uo[o] +
                         c2 * (p117_u[o - 1] + p117_u[o + 1] +
                               p117_u[o - P117_NX] + p117_u[o + P117_NX] -
                               4.0f * p117_u[o])) * dmp;
            }
        /* three drivers on slow orbits, always smooth, never impulsive */
        for (i = 0; i < 3; i++) {
            float a = 0.00061f * t * (1.0f + 0.27f * (float)i) + (float)i * 2.1f;
            float b = 0.00044f * t * (1.3f - 0.19f * (float)i) + (float)i * 1.3f;
            float dx = (float)P117_NX * (0.5f + 0.30f * sinf(a));
            float dy = (float)P117_NY * (0.5f + 0.30f * sinf(b));
            float amp = 0.00035f * sinf((0.0125f + 0.0021f * (float)i) * t);
            int xi, yi;
            for (yi = 1; yi < P117_NY - 1; yi++)
                for (xi = 1; xi < P117_NX - 1; xi++) {
                    float ex = (float)xi - dx, ey = (float)yi - dy;
                    float r2 = ex * ex + ey * ey;
                    if (r2 > 95.0f) continue;
                    un[yi * P117_NX + xi] += amp * expf(-r2 * 0.055f);
                }
        }
        /* viscosity: smooth the velocity field, which damps the high modes
         * hard and leaves the slow, large modes alone — no jitter, no strobe */
        for (j = 1; j < P117_NY - 1; j++)
            for (i = 1; i < P117_NX - 1; i++) {
                o = j * P117_NX + i;
                vv[o] = un[o] - p117_u[o];
            }
        for (j = 1; j < P117_NY - 1; j++)
            for (i = 1; i < P117_NX - 1; i++) {
                float v;
                o = j * P117_NX + i;
                v = 0.60f * vv[o] + 0.10f * (vv[o - 1] + vv[o + 1] +
                        vv[o - P117_NX] + vv[o + P117_NX]);
                p117_uo[o] = p117_u[o];
                p117_u[o] = p117_u[o] + v;
            }
        /* automatic gain control: coherent driving would otherwise pump a
         * resonant mode without bound. Energy is measured and the field eased
         * toward a fixed RMS at a quarter percent per frame — far too slow to see, but it
         * pins the motion budget for any run length. */
        {
            float e = 0.0f, rms, g;
            for (j = 1; j < P117_NY - 1; j++)
                for (i = 1; i < P117_NX - 1; i++) {
                    o = j * P117_NX + i;
                    e += p117_u[o] * p117_u[o];
                }
            rms = sqrtf(e / (float)((P117_NX - 2) * (P117_NY - 2)));
            if (rms > 1e-7f) {
                g = 0.0205f / rms;
                g = 1.0f + (g - 1.0f) * 0.01f;
                if (g < 0.9975f) g = 0.9975f;
                if (g > 1.0045f) g = 1.0045f;
                for (j = 1; j < P117_NY - 1; j++)
                    for (i = 1; i < P117_NX - 1; i++) {
                        o = j * P117_NX + i;
                        p117_u[o] *= g; p117_uo[o] *= g;
                    }
            }
            /* explicit spatial dissipation: a light diffusion of the
             * displacement itself. Long modes (the ones we want to watch) are
             * untouched; anything approaching grid-Nyquist loses 12% a frame,
             * which is what keeps the wireframe from ever shimmering. */
            for (j = 1; j < P117_NY - 1; j++)
                for (i = 1; i < P117_NX - 1; i++) {
                    o = j * P117_NX + i;
                    vv[o] = 0.94f * p117_u[o] + 0.015f * (p117_u[o - 1] +
                            p117_u[o + 1] + p117_u[o - P117_NX] +
                            p117_u[o + P117_NX]);
                    un[o] = 0.94f * p117_uo[o] + 0.015f * (p117_uo[o - 1] +
                            p117_uo[o + 1] + p117_uo[o - P117_NX] +
                            p117_uo[o + P117_NX]);
                }
            for (j = 1; j < P117_NY - 1; j++)
                for (i = 1; i < P117_NX - 1; i++) {
                    o = j * P117_NX + i;
                    p117_u[o] = vv[o]; p117_uo[o] = un[o];
                }
            /* soft per-node limiter: a driver sitting on one spot could dig a
             * narrow well far deeper than the global RMS. tanh saturation caps
             * the relief without ever introducing a corner. */
            for (j = 1; j < P117_NY - 1; j++)
                for (i = 1; i < P117_NX - 1; i++) {
                    o = j * P117_NX + i;
                    p117_u[o] = 0.042f * tanhf(p117_u[o] * (1.0f / 0.042f));
                    p117_uo[o] = 0.042f * tanhf(p117_uo[o] * (1.0f / 0.042f));
                }
        }
    }

    memset(p117_acc, 0, sizeof p117_acc);
    yaw   = 0.00058f * t;
    cy_   = cosf(yaw); sy_ = sinf(yaw);
    pitch = 0.92f + 0.13f * sinf(0.00039f * t);
    cp    = cosf(pitch); spn = sinf(pitch);
    hs    = 11.5f;
    scale = 900.0f;
    hbase = (int)(t * 0.042f + sp * 30.0f);

    for (j = 0; j < P117_NY; j++)
        for (i = 0; i < P117_NX; i++) {
            float X = ((float)i / (float)(P117_NX - 1) - 0.5f) * 2.0f;
            float Y = ((float)j / (float)(P117_NY - 1) - 0.5f) * 2.0f;
            float H = p117_u[j * P117_NX + i] * hs;
            float Xr = X * cy_ - Y * sy_;
            float Yr = X * sy_ + Y * cy_;
            float D  = Yr * cp - H * spn + 3.25f;
            float V  = Yr * spn + H * cp;
            float s  = 1.0f / D;
            o = j * P117_NX + i;
            p117_px[o] = (float)P117_LW * 0.5f + Xr * s * scale;
            p117_py[o] = (float)P117_LH * 0.5f - V * s * scale * 0.92f;
            p117_pd[o] = D - 3.25f;
        }

    for (j = 0; j < P117_NY; j++)
        for (i = 0; i < P117_NX; i++) {
            o = j * P117_NX + i;
            if (i + 1 < P117_NX) p117_wire(o, o + 1, hbase, t);
            if (j + 1 < P117_NY) p117_wire(o, o + P117_NX, hbase, t);
        }
    p117_blit(fb, w, h);
}
