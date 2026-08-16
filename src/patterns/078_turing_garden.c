/* 078 Turing Garden — reaction-diffusion continents with gold rims.
 * Port of lab/patterns/078_turing_garden/proto.py. The lab renderer rebuilds
 * the field from a drifting sine seed and iterates f <- tanh(2.35*blur3(f))
 * k times per frame; per the spec's ARM64 plan this port keeps the field
 * persistent and runs 2 iterations per frame with a slowly decaying seed
 * injection (same attractor, 14x cheaper, no frame-to-frame jumps).
 * Field is 320x240, duotone + rim colouring, bilinear upscale to (w,h). */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GW 320
#define GH 240
#define ANGK 651.8986f              /* 4096 / 2pi */

static float *fld, *fld2, *dspf;
static uint8_t (*img)[GW][3];
static float sintab[4096];
static float tanhlut[2048];
static float rimlut[256];
static int ix1[GW], a3x[GW], iy2[GH], b3y[GH];
static int *irad;
static float sx5[GW];
static uint8_t vign[GH][GW];
static float huetab[1024][3];
static int inited;
static int last_sl = -1;
static int up_w = -1;
static int *up_xi;
static uint8_t *up_fx;

/* hue angle of each sampled palette entry; the duotone pair is built by
 * rotating that angle, so the two domains stay complementary in any scheme */
static void build_huetab(const uint32_t *pal) {
    for (int i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255), b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b;
        float mn = r < g ? r : g; if (b < mn) mn = b;
        float d = mx - mn, hu;
        if (d < 1.0f)      hu = 0.0f;
        else if (mx == r)  hu = (g - b) / d / 6.0f;
        else if (mx == g)  hu = (2.0f + (b - r) / d) / 6.0f;
        else               hu = (4.0f + (r - g) / d) / 6.0f;
        if (hu < 0.0f) hu += 1.0f;
        huetab[i][0] = hu;
    }
}
static void hsvcol(float hue, float sat, float val, float *out) {
    hue = hue - floorf(hue);
    float h6 = hue * 6.0f;
    int    i = (int)h6;
    float  f = h6 - (float)i;
    float p = val * (1.0f - sat), q = val * (1.0f - sat * f), w = val * (1.0f - sat * (1.0f - f));
    switch (i % 6) {
        case 0: out[0] = val; out[1] = w;   out[2] = p;   break;
        case 1: out[0] = q;   out[1] = val; out[2] = p;   break;
        case 2: out[0] = p;   out[1] = val; out[2] = w;   break;
        case 3: out[0] = p;   out[1] = q;   out[2] = val; break;
        case 4: out[0] = w;   out[1] = p;   out[2] = val; break;
        default:out[0] = val; out[1] = p;   out[2] = q;   break;
    }
}
static float lsin(float a) { return sintab[((int)(a * ANGK + 4096.5f)) & 4095]; }
static float ltanh(float x) {
    int i = (int)(x * 128.0f) + 1024;
    if (i < 0) i = 0; if (i > 2047) i = 2047;
    return tanhlut[i];
}

static void upscale(uint32_t *fb, int w, int h) {
    if (w != up_w) {
        free(up_xi); free(up_fx);
        up_xi = (int *)malloc(sizeof(int) * (size_t)w);
        up_fx = (uint8_t *)malloc((size_t)w);
        for (int x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8; if (xi > GW - 2) { xi = GW - 2; q = (GW - 1) * 256; }
            up_xi[x] = xi * 3; up_fx[x] = (uint8_t)(q & 255);
        }
        up_w = w;
    }
    for (int y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8; if (yi > GH - 2) { yi = GH - 2; qy = (GH - 1) * 256; }
        int fy = qy & 255;
        const uint8_t *r0 = &img[yi][0][0], *r1 = &img[yi + 1][0][0];
        uint32_t *out = fb + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            int X = up_xi[x], fx = up_fx[x];
            int c[3];
            for (int k = 0; k < 3; k++) {
                int t0 = r0[X+k] + (((r0[X+3+k] - r0[X+k]) * fx) >> 8);
                int t1 = r1[X+k] + (((r1[X+3+k] - r1[X+k]) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

static void init_tables(void) {
    for (int i = 0; i < 4096; i++) sintab[i] = sinf((float)i * (6.2831853f / 4096.0f));
    for (int i = 0; i < 2048; i++) tanhlut[i] = tanhf(((float)i - 1024.0f) / 128.0f);
    for (int i = 0; i < 256; i++) {
        float r = (float)i / 255.0f;
        rimlut[i] = powf(r, 1.5f);
    }
    for (int x = 0; x < GW; x++) {
        ix1[x] = (int)((float)x * 0.085f * ANGK + 4096.5f);
        a3x[x] = (int)((float)x * 0.71f * 0.070f * ANGK);
    }
    for (int y = 0; y < GH; y++) {
        iy2[y] = (int)((float)y * 0.079f * ANGK + 4096.5f);
        b3y[y] = (int)((float)y * 0.65f * 0.070f * ANGK);
    }
    fld  = malloc(sizeof(float) * GW * GH);
    dspf = malloc(sizeof(float) * GW * GH);
    fld2 = malloc(sizeof(float) * GW * GH);
    img  = malloc(GH * GW * 3);
    irad = malloc(sizeof(int) * GW * GH);
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            float r = hypotf((float)x - GW / 2.0f, (float)y - GH / 2.0f);
            irad[y * GW + x] = (int)(r * 0.10f * ANGK);
            float dx = ((float)x - GW / 2.0f) / GW, dy = ((float)y - GH / 2.0f) / GH;
            float v = 1.0f - 0.35f * (dx * dx + dy * dy);
            vign[y][x] = (uint8_t)(v * 255.0f);
        }
}

/* drifting sine seed, written into dst */
static void build_seed(float *dst, float ph) {
    int ip1 = (int)(ph * ANGK);
    int ip2 = (int)(-ph * 0.8f * ANGK);
    int ip3 = (int)(ph * 0.6f * ANGK) + 4096;
    int ip4 = (int)(-ph * 0.5f * ANGK) + 4096;
    for (int x = 0; x < GW; x++)
        sx5[x] = lsin((float)x * 0.21f - ph * 0.33f);
    for (int y = 0; y < GH; y++) {
        float t2 = sintab[(iy2[y] + ip2) & 4095];
        float t5 = 1.1f * lsin((float)y * 0.19f + ph * 0.41f);
        int by = b3y[y] + ip3;
        const int *rr = irad + y * GW;
        float *d = dst + y * GW;
        for (int x = 0; x < GW; x++) {
            float s = sintab[(ix1[x] + ip1) & 4095] + t2
                    + sintab[(a3x[x] + by) & 4095]
                    + sintab[(rr[x] + ip4) & 4095]
                    + t5 * sx5[x];
            d[x] = s * (1.0f / 4.5f);
        }
    }
}

/* f <- tanh(2.35 * blur3(f)) on the wrapped 320x240 grid */
static void relax(void) {
    for (int y = 0; y < GH; y++) {
        const float *r  = fld + y * GW;
        const float *rm = fld + ((y + GH - 1) % GH) * GW;
        const float *rp = fld + ((y + 1) % GH) * GW;
        float *o = fld2 + y * GW;
        for (int x = 0; x < GW; x++) {
            int xm = x ? x - 1 : GW - 1, xp = (x + 1 == GW) ? 0 : x + 1;
            float b = r[x] * 0.4f + 0.15f * (rm[x] + rp[x] + r[xm] + r[xp]);
            o[x] = ltanh(b * 2.35f);
        }
    }
    float *tmp = fld; fld = fld2; fld2 = tmp;
}

void pattern_078(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!inited) { init_tables(); inited = 1; last_sl = -1; }
    build_huetab(pal);
    float t = (float)frame;
    float ph = t * 0.006f;

    int fresh = (last_sl < 0 || (sl == 0 && last_sl != 0));
    if (fresh) {
        build_seed(fld, ph);
        for (int i = 0; i < 10; i++) relax();
    }
    last_sl = sl;

    /* seed injection: strong while the colonies are young, then settles */
    float inj = 0.30f - (float)sl * 0.0006f;
    if (inj < 0.055f) inj = 0.055f;
    build_seed(fld2, ph);
    for (int i = 0; i < GW * GH; i++)
        fld[i] += (fld2[i] - fld[i]) * inj;
    relax();
    relax();

    /* TEMPORAL REVIEW 2.4.0 (docs/review/04_pattern_temporal.md, F-078):
     * the tanh reaction field is bistable — a colony boundary flips a
     * region between the two states within a couple of frames, which reads
     * as repeated delta ~2 snaps on a 0.13 median.  Render through a
     * temporal EWMA of the field (1/4 per frame): the dynamics are
     * untouched, but every snap reaches the screen as a ~7-frame dissolve. */
    if (fresh) memcpy(dspf, fld, sizeof(float) * GW * GH);
    else for (int i = 0; i < GW * GH; i++) dspf[i] += (fld[i] - dspf[i]) * 0.25f;

    /* duotone + gold rim + vignette */
    float hsel = 0.52f + 0.08f * lsin(t * 0.003f)
               + (float)((seed >> 9) & 1023) * (1.0f / 1024.0f);
    float hueA = huetab[(int)(hsel * 1024.0f + 1024.0f) & 1023][0];
    float cA[3], cB[3];
    hsvcol(hueA, 0.9f, 0.55f, cA);
    hsvcol(hueA + 0.42f, 0.85f, 0.95f, cB);
    /* F-078 (cont.): huetab collapses near-grey palette entries to hue 0,
     * so as hsel walks the palette the duotone endpoints could flip in ONE
     * frame (single 2.4-delta whole-field recolour).  EWMA the endpoints:
     * a flip becomes a ~20-frame blend, ordinary drift is untouched. */
    {
        static float cAs[3], cBs[3]; static int cok;
        if (fresh || !cok) { memcpy(cAs, cA, sizeof cAs); memcpy(cBs, cB, sizeof cBs); cok = 1; }
        for (int k = 0; k < 3; k++) {
            cAs[k] += (cA[k] - cAs[k]) * 0.10f; cA[k] = cAs[k];
            cBs[k] += (cB[k] - cBs[k]) * 0.10f; cB[k] = cBs[k];
        }
    }
    /* rims stay a fixed warm gold in every scheme (spec: the cohesive constant) */
    float gold[3] = { 1.0f, 0.85f, 0.35f };
    float pulse = (0.75f + 0.25f * lsin(t * 0.02f)) * 0.8f;
    for (int k = 0; k < 3; k++) gold[k] *= pulse;

    for (int y = 0; y < GH; y++) {
        const float *r  = dspf + y * GW;
        const float *rm = dspf + ((y + GH - 1) % GH) * GW;
        const float *rp = dspf + ((y + 1) % GH) * GW;
        for (int x = 0; x < GW; x++) {
            int xm = x ? x - 1 : GW - 1, xp = (x + 1 == GW) ? 0 : x + 1;
            float u = (r[x] + 1.0f) * 0.5f;
            float gx = (r[xp] - r[xm]) * 0.5f, gy = (rp[x] - rm[x]) * 0.5f;
            float rim = fabsf(gx) + fabsf(gy);
            if (rim > 1.0f) rim = 1.0f;
            float rg = rimlut[(int)(rim * 255.0f)];
            float vf = (float)vign[y][x] * (1.0f / 255.0f);
            for (int k = 0; k < 3; k++) {
                float c = (cA[k] + (cB[k] - cA[k]) * u + rg * gold[k]) * vf;
                int v = (int)(c * 255.0f);
                img[y][x][k] = (uint8_t)(v > 255 ? 255 : (v < 0 ? 0 : v));
            }
        }
    }
    upscale(fb, w, h);
}
