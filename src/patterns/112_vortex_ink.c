/* 112 Vortex Ink — a real incompressible fluid, dyed.
 * Stam's stable-fluids solver on a 208x156 grid: semi-Lagrangian advection of
 * both velocity and dye, then a 16-iteration Jacobi pressure solve whose
 * gradient is subtracted to force div(u) = 0, plus a vorticity-confinement term
 * that feeds energy back into the small eddies the solver would otherwise damp
 * out. Three stirrers orbit the tank, each adding a swirl of velocity and
 * injecting its own colour of ink; because the flow is genuinely
 * divergence-free the ink sheets fold, stretch and roll up into spirals instead
 * of merely drifting. Unconditionally stable, so it can never blow up. Dense,
 * soft-edged, slow: a ground layer.
 */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

#define P112_W 208
#define P112_H 156
#define P112_SW (P112_W + 2)
#define P112_SH (P112_H + 2)
#define P112_N  (P112_SW * P112_SH)
#define P112_IX(i, j) ((i) + P112_SW * (j))

static float p112_u[P112_N], p112_v[P112_N], p112_u0[P112_N], p112_v0[P112_N];
static float p112_d[3][P112_N], p112_d0[3][P112_N];
static float p112_p[P112_N], p112_div[P112_N], p112_cu[P112_N];
static int p112_init;
static unsigned char p112_img[P112_W * P112_H * 3];
static float p112_ramp[256][3];
static jd_up p112_up;

static void p112_ramp_build(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i * 128) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 8.0f) mx = 8.0f;
        p112_ramp[i][0] = r / mx; p112_ramp[i][1] = g / mx; p112_ramp[i][2] = b / mx;
    }
}

/* b = 1 mirrors x-velocity at the walls, 2 mirrors y, 0 is a scalar */
static void p112_bnd(int b, float *f)
{
    int i;
    for (i = 1; i <= P112_H; i++) {
        f[P112_IX(0, i)]           = b == 1 ? -f[P112_IX(1, i)] : f[P112_IX(1, i)];
        f[P112_IX(P112_W + 1, i)]  = b == 1 ? -f[P112_IX(P112_W, i)]
                                            : f[P112_IX(P112_W, i)];
    }
    for (i = 1; i <= P112_W; i++) {
        f[P112_IX(i, 0)]           = b == 2 ? -f[P112_IX(i, 1)] : f[P112_IX(i, 1)];
        f[P112_IX(i, P112_H + 1)]  = b == 2 ? -f[P112_IX(i, P112_H)]
                                            : f[P112_IX(i, P112_H)];
    }
    f[P112_IX(0, 0)] = 0.5f * (f[P112_IX(1, 0)] + f[P112_IX(0, 1)]);
    f[P112_IX(0, P112_H + 1)] = 0.5f * (f[P112_IX(1, P112_H + 1)] +
                                        f[P112_IX(0, P112_H)]);
    f[P112_IX(P112_W + 1, 0)] = 0.5f * (f[P112_IX(P112_W, 0)] +
                                        f[P112_IX(P112_W + 1, 1)]);
    f[P112_IX(P112_W + 1, P112_H + 1)] = 0.5f * (f[P112_IX(P112_W, P112_H + 1)] +
                                                 f[P112_IX(P112_W + 1, P112_H)]);
}

static void p112_advect(int b, float *d, const float *d0,
                        const float *u, const float *v, float dt)
{
    int i, j;
    for (j = 1; j <= P112_H; j++)
        for (i = 1; i <= P112_W; i++) {
            int o = P112_IX(i, j), i0, j0;
            float x = (float)i - dt * u[o];
            float y = (float)j - dt * v[o];
            float s1, s0, t1, t0;
            if (x < 0.5f) x = 0.5f; if (x > (float)P112_W + 0.5f) x = (float)P112_W + 0.5f;
            if (y < 0.5f) y = 0.5f; if (y > (float)P112_H + 0.5f) y = (float)P112_H + 0.5f;
            i0 = (int)x; j0 = (int)y;
            s1 = x - (float)i0; s0 = 1.0f - s1;
            t1 = y - (float)j0; t0 = 1.0f - t1;
            d[o] = s0 * (t0 * d0[P112_IX(i0, j0)] + t1 * d0[P112_IX(i0, j0 + 1)]) +
                   s1 * (t0 * d0[P112_IX(i0 + 1, j0)] + t1 * d0[P112_IX(i0 + 1, j0 + 1)]);
        }
    p112_bnd(b, d);
}

static void p112_project(void)
{
    int i, j, k;
    for (j = 1; j <= P112_H; j++)
        for (i = 1; i <= P112_W; i++) {
            int o = P112_IX(i, j);
            p112_div[o] = -0.5f * (p112_u[o + 1] - p112_u[o - 1] +
                                   p112_v[o + P112_SW] - p112_v[o - P112_SW]);
            p112_p[o] = 0.0f;
        }
    p112_bnd(0, p112_div); p112_bnd(0, p112_p);
    for (k = 0; k < 16; k++) {
        for (j = 1; j <= P112_H; j++)
            for (i = 1; i <= P112_W; i++) {
                int o = P112_IX(i, j);
                p112_p[o] = (p112_div[o] + p112_p[o - 1] + p112_p[o + 1] +
                             p112_p[o - P112_SW] + p112_p[o + P112_SW]) * 0.25f;
            }
        p112_bnd(0, p112_p);
    }
    for (j = 1; j <= P112_H; j++)
        for (i = 1; i <= P112_W; i++) {
            int o = P112_IX(i, j);
            p112_u[o] -= 0.5f * (p112_p[o + 1] - p112_p[o - 1]);
            p112_v[o] -= 0.5f * (p112_p[o + P112_SW] - p112_p[o - P112_SW]);
        }
    p112_bnd(1, p112_u); p112_bnd(2, p112_v);
}

/* vorticity confinement — puts back the swirl the advection eats */
static void p112_vort(float eps)
{
    int i, j;
    for (j = 1; j <= P112_H; j++)
        for (i = 1; i <= P112_W; i++) {
            int o = P112_IX(i, j);
            float c = 0.5f * (p112_v[o + 1] - p112_v[o - 1] -
                              p112_u[o + P112_SW] + p112_u[o - P112_SW]);
            p112_cu[o] = c < 0.0f ? -c : c;
            p112_p[o] = c;                       /* reuse p as signed curl */
        }
    for (j = 2; j < P112_H; j++)
        for (i = 2; i < P112_W; i++) {
            int o = P112_IX(i, j);
            float gx = 0.5f * (p112_cu[o + 1] - p112_cu[o - 1]);
            float gy = 0.5f * (p112_cu[o + P112_SW] - p112_cu[o - P112_SW]);
            float m = sqrtf(gx * gx + gy * gy) + 1e-5f;
            gx /= m; gy /= m;
            p112_u[o] += eps * gy * p112_p[o];
            p112_v[o] -= eps * gx * p112_p[o];
        }
}

void pattern_112(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sp = (float)(seed & 1023) * 0.006136f;
    int i, j, c, hbase;
    (void)sl;

    p112_ramp_build(pal);
    if (!p112_init) {
        memset(p112_u, 0, sizeof p112_u);   memset(p112_v, 0, sizeof p112_v);
        memset(p112_d, 0, sizeof p112_d);
        p112_init = 1;
    }
    hbase = (int)(t * 0.033f + sp * 30.0f);

    /* three stirrers: a swirl of velocity and a plume of ink each */
    for (i = 0; i < 3; i++) {
        float fi = (float)i;
        float a = 0.00097f * t * (1.0f + 0.31f * fi) + fi * 2.09f + sp;
        float b = 0.00071f * t * (1.3f - 0.19f * fi) + fi * 1.31f;
        float sx = (float)P112_W * (0.5f + 0.31f * sinf(a));
        float sy = (float)P112_H * (0.5f + 0.31f * sinf(b));
        float dir = 0.0043f * t * (i == 1 ? -1.0f : 1.0f) + fi * 2.0f;
        float fx = cosf(dir) * 2.6f, fy = sinf(dir) * 2.6f;
        const float *col = p112_ramp[(hbase + i * 74) & 255];
        int xi, yi;
        for (yi = -7; yi <= 7; yi++)
            for (xi = -7; xi <= 7; xi++) {
                int gx = (int)sx + xi, gy = (int)sy + yi;
                float r2 = (float)(xi * xi + yi * yi);
                float g;
                int o;
                if (gx < 1 || gy < 1 || gx > P112_W || gy > P112_H) continue;
                if (r2 > 49.0f) continue;
                o = P112_IX(gx, gy);
                g = expf(-r2 * 0.11f);
                p112_u[o] += fx * g * 0.55f;
                p112_v[o] += fy * g * 0.55f;
                for (c = 0; c < 3; c++) p112_d[c][o] += col[c] * g * 0.155f;
            }
    }

    p112_vort(0.32f);
    p112_project();

    memcpy(p112_u0, p112_u, sizeof p112_u);
    memcpy(p112_v0, p112_v, sizeof p112_v);
    p112_advect(1, p112_u, p112_u0, p112_u0, p112_v0, 1.0f);
    p112_advect(2, p112_v, p112_v0, p112_u0, p112_v0, 1.0f);
    p112_project();

    for (c = 0; c < 3; c++) {
        memcpy(p112_d0[c], p112_d[c], sizeof p112_d0[c]);
        p112_advect(0, p112_d[c], p112_d0[c], p112_u, p112_v, 1.0f);
    }
    /* gentle drag and ink fade keep the tank from saturating */
    for (j = 1; j <= P112_H; j++)
        for (i = 1; i <= P112_W; i++) {
            int o = P112_IX(i, j);
            p112_u[o] *= 0.9955f; p112_v[o] *= 0.9955f;
            for (c = 0; c < 3; c++) p112_d[c][o] *= 0.9955f;
        }

    /* tone map the ink into a small image, then bilinear-upscale */
    for (j = 0; j < P112_H; j++)
        for (i = 0; i < P112_W; i++) {
            int o = P112_IX(i + 1, j + 1);
            for (c = 0; c < 3; c++) {
                float v = 255.0f * p112_d[c][o] / (0.42f + p112_d[c][o]);
                p112_img[(j * P112_W + i) * 3 + c] =
                    v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
            }
        }
    jd_up_blit(&p112_up, fb, w, h, p112_img, P112_W, P112_H);
}
