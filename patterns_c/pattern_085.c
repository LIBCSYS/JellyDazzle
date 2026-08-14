/* 085 Vector Machine — CGA "H-frame" poster: giant concentric arc stacks left
 * and right, an X strut lattice, a gradient H of panels and a breathing diamond
 * at dead centre, all on black.
 * Port of lab/patterns/085_vector_machine/proto.py. Deliberately duotone: three
 * widely separated palette entries stand in for the original red / blue-magenta
 * / white triad, and they cycle slowly (the routine's DAC roll).
 * Repaint pattern, full resolution, per-frame ring + ramp LUTs. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P85_TAU 6.28318530718f

static float p85_ptab[1024][3];
static float p85_sin[2048];
static float p85_arcL[1024], p85_arcR[1024];
static int p85_inited;

/* The two arc stacks are a fixed annulus in screen space: both the sqrtf and
 * the 55<r<170 test depend only on (x,y). Bake the LUT index per pixel once
 * (-1 == outside the annulus); the arc *values* still vary per frame. */
static int16_t *p85_kL, *p85_kR;
static int p85_tw, p85_th;
static int16_t p85_rowL[4096], p85_rowR[4096];  /* fallback if alloc fails */

static void p85_arcrow(int16_t *kl, int16_t *kr, float qy, float isx, int w)
{
    int x;
    if (w > 4096) w = 4096;
    for (x = 0; x < w; x++) {
        float px = ((float)x + 0.5f) * isx - 160.0f;
        float dx = px + 175.0f;
        int k = -1;
        if (dx > 0.0f) {
            float r = sqrtf(qy + dx * dx);
            if (r > 55.0f && r < 170.0f) {
                k = (int)(r * 4.0f); if (k > 1023) k = 1023;
            }
        }
        kl[x] = (int16_t)k;
        dx = px - 175.0f; k = -1;
        if (dx < 0.0f) {
            float r = sqrtf(qy + dx * dx);
            if (r > 55.0f && r < 170.0f) {
                k = (int)(r * 4.0f); if (k > 1023) k = 1023;
            }
        }
        kr[x] = (int16_t)k;
    }
}

static void p85_map(int w, int h)
{
    float isx, isy;
    int y;
    if (p85_tw == w && p85_th == h && p85_kL) return;
    free(p85_kL); free(p85_kR);
    p85_kL = (int16_t *)malloc(sizeof(int16_t) * (size_t)w * (size_t)h);
    p85_kR = (int16_t *)malloc(sizeof(int16_t) * (size_t)w * (size_t)h);
    if (!p85_kL || !p85_kR) {
        free(p85_kL); free(p85_kR); p85_kL = 0; p85_kR = 0;
        p85_tw = 0; p85_th = 0; return;
    }
    isx = 320.0f / (float)w; isy = 240.0f / (float)h;
    for (y = 0; y < h; y++) {
        float py = ((float)y + 0.5f) * isy - 120.0f;
        p85_arcrow(p85_kL + (long)y * w, p85_kR + (long)y * w,
                   py * py, isx, w);
    }
    p85_tw = w; p85_th = h;
}

static void p85_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p85_sin[i] = sinf((float)i * (P85_TAU / 2048.0f));
    p85_inited = 1;
}

static void p85_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 24.0f) mx = 24.0f;
        p85_ptab[i][0] = r / mx; p85_ptab[i][1] = g / mx; p85_ptab[i][2] = b / mx;
    }
}

static float p85_lsin(float a)
{
    return p85_sin[((int)(a * 325.949318f + 32768.5f)) & 2047];
}

void pattern_085(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float isx = 320.0f / (float)w, isy = 240.0f / (float)h;
    const float *cA, *cB, *cC;
    float grow = t * 0.004f, ds;
    float ramp[3];
    int base, i, x, y;
    (void)sl;

    if (!p85_inited) p85_init();
    p85_buildpal(pal);

    base = (int)(t * 0.4f) + (int)((seed >> 9) & 1023);
    cA = p85_ptab[base & 1023];              /* arcs + struts (the "red") */
    cB = p85_ptab[(base + 300) & 1023];      /* H ramp start */
    cC = p85_ptab[(base + 780) & 1023];      /* H ramp end */

    for (i = 0; i < 1024; i++) {
        float r = (float)i * 0.25f;
        p85_arcL[i] = 0.5f + 0.5f * p85_lsin(r * 0.26f + t * 0.010f);
        p85_arcR[i] = 0.5f + 0.5f * p85_lsin(r * 0.26f - t * 0.010f);
    }
    ds = 14.0f + 3.0f * p85_lsin(t * 0.02f);

    p85_map(w, h);

    for (y = 0; y < h; y++) {
        float py = ((float)y + 0.5f) * isy - 120.0f;
        float ay = fabsf(py), qy = py * py;
        float g = (py + 70.0f) * (1.0f / 140.0f) + grow;
        uint32_t *row = fb + (long)y * w;
        const int16_t *kl, *kr;
        int inbar = (ay < 72.0f), incross = (ay < 13.0f);
        if (p85_kL) { kl = p85_kL + (long)y * w; kr = p85_kR + (long)y * w; }
        else { p85_arcrow(p85_rowL, p85_rowR, qy, isx, w);
               kl = p85_rowL; kr = p85_rowR; }
        g -= floorf(g);
        ramp[0] = cB[0] + (cC[0] - cB[0]) * g;
        ramp[1] = cB[1] + (cC[1] - cB[1]) * g;
        ramp[2] = cB[2] + (cC[2] - cB[2]) * g;
        for (x = 0; x < w; x++) {
            float px = ((float)x + 0.5f) * isx - 160.0f;
            float ax = fabsf(px);
            float cr = 0.0f, cg = 0.0f, cb = 0.0f;
            float m, d, dman, dia;
            int ir, ig, ib, k;

            /* red X lattice */
            {
                d = fabsf(py - 0.613105f * px) * 0.852525f;
                m = 1.0f - d * 0.5f; if (m > 0.0f) { if (m > 1.0f) m = 1.0f;
                    cr += m*0.45f*cA[0]; cg += m*0.45f*cA[1]; cb += m*0.45f*cA[2]; }
                d = fabsf(py + 0.613105f * px) * 0.852525f;
                m = 1.0f - d * 0.5f; if (m > 0.0f) { if (m > 1.0f) m = 1.0f;
                    cr += m*0.45f*cA[0]; cg += m*0.45f*cA[1]; cb += m*0.45f*cA[2]; }
                d = fabsf(py - 1.743315f * px) * 0.497571f;
                m = 1.0f - d * 0.5f; if (m > 0.0f) { if (m > 1.0f) m = 1.0f;
                    cr += m*0.45f*cA[0]; cg += m*0.45f*cA[1]; cb += m*0.45f*cA[2]; }
                d = fabsf(py + 1.743315f * px) * 0.497571f;
                m = 1.0f - d * 0.5f; if (m > 0.0f) { if (m > 1.0f) m = 1.0f;
                    cr += m*0.45f*cA[0]; cg += m*0.45f*cA[1]; cb += m*0.45f*cA[2]; }
            }

            /* concentric arc stacks; keep only the inward-facing half */
            k = kl[x];
            if (k >= 0) {
                m = p85_arcL[k] * 0.85f;
                cr += m * cA[0]; cg += m * cA[1]; cb += m * cA[2];
            }
            k = kr[x];
            if (k >= 0) {
                m = p85_arcR[k] * 0.85f;
                cr += m * cA[0]; cg += m * cA[1]; cb += m * cA[2];
            }

            /* the H: two gradient uprights, then the crossbar */
            if (inbar && ax > 20.0f && ax < 60.0f) {
                cr = ramp[0]; cg = ramp[1]; cb = ramp[2];
            }
            if (incross && ax < 40.0f) {
                float gc = px * (1.0f / 60.0f) + grow;
                gc -= floorf(gc);
                cr = cC[0] + (cB[0] - cC[0]) * gc;
                cg = cC[1] + (cB[1] - cC[1]) * gc;
                cb = cC[2] + (cB[2] - cC[2]) * gc;
            }

            /* breathing white diamond */
            dman = ax + ay;
            if (dman < ds + 2.0f) {
                dia = 1.0f - fabsf(dman - ds) * 0.5f;
                if (dia < 0.0f) dia = 0.0f;
                if (dman < ds) dia += 0.75f;
                cr += dia * 0.81f; cg += dia * 0.765f; cb += dia * 0.855f;
            }

            ir = (int)(cr * 255.0f); if (ir > 255) ir = 255; if (ir < 0) ir = 0;
            ig = (int)(cg * 255.0f); if (ig > 255) ig = 255; if (ig < 0) ig = 0;
            ib = (int)(cb * 255.0f); if (ib > 255) ib = 255; if (ib < 0) ib = 0;
            row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                   | ((uint32_t)ig << 8) | (uint32_t)ib;
        }
    }
}
