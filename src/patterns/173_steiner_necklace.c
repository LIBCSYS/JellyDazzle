/* 173 Steiner Necklace — nested rings of tangent circles under an inversion.
 * A Steiner chain is the closed ring of n circles that fits between two given
 * circles, each touching its neighbours; in the concentric case the geometry is
 * exact — centre distance 1, chain radius sin(pi/n), boundaries 1 +/- sin(pi/n).
 * Steiner's porism says the chain can be rotated freely and stays closed, so it
 * turns forever. The inner boundary of one chain is used as the outer boundary
 * of the next, three deep, and the whole nest is then pushed through a circle
 * inversion whose centre drifts outside the figure: inversion preserves circles
 * and tangency, so the necklace stays exactly tangent while going wildly
 * eccentric — small pearls crowding one side, big lazy ones opposite. Rings and
 * tangency sparks only; everything between them is black. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p173_up;

#define P173_W 480
#define P173_H 360
#define P173_NCH 3
#define P173_TAU 6.28318530717958647692f

static float p173_acc[P173_W * P173_H * 3];
static unsigned char p173_img[P173_W * P173_H * 3];
static unsigned char p173_tone[1024];
static int *p173_xm;
static int p173_xmw;
static int p173_ready;
static uint32_t p173_seedc;
static float p173_col[64][3];
static float p173_hue0, p173_huew, p173_orb, p173_pow, p173_spin;
static int p173_n[P173_NCH];

static uint32_t p173_rs;
static float p173_rf(void)
{
    p173_rs ^= p173_rs << 13; p173_rs ^= p173_rs >> 17; p173_rs ^= p173_rs << 5;
    return (float)(p173_rs >> 8) * (1.0f / 16777216.0f);
}

static void p173_setup(uint32_t seed)
{
    int i;
    p173_rs = seed ? seed ^ 0x5731E1A2u : 0x5731E1A2u;
    p173_rf(); p173_rf();
    p173_hue0  = p173_rf();
    p173_huew  = 0.12f + p173_rf() * 0.50f;
    p173_orb   = 1.55f + p173_rf() * 0.55f;       /* inversion centre radius */
    p173_pow   = 1.30f + p173_rf() * 1.30f;       /* inversion power k^2     */
    p173_spin  = (p173_rf() < 0.5f ? -1.0f : 1.0f) * (0.0016f + p173_rf() * 0.0016f);
    for (i = 0; i < P173_NCH; i++)
        p173_n[i] = 5 + (int)(p173_rf() * 8.0f);  /* 5..12 pearls per chain  */
    if (!p173_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (4.4f / 1024.0f)));
            p173_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p173_ready = 1;
    }
    p173_seedc = seed;
}

static void p173_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 64; i++) {
        float hue = p173_hue0 + p173_huew * ((float)i / 63.0f);
        float r, g, b, mx;
        uint32_t p;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p173_col[i][0] = 0.14f + 0.86f * r / mx;
        p173_col[i][1] = 0.14f + 0.86f * g / mx;
        p173_col[i][2] = 0.14f + 0.86f * b / mx;
    }
}

static void p173_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= P173_W - 1 || yi >= P173_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p173_acc + (yi * P173_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P173_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

/* one glowing ring, sampled at ~1 px spacing, brightness w */
static void p173_ring(float cx, float cy, float rad, const float *c, float w)
{
    int ns, i;
    float da, a;
    if (rad < 0.4f || rad > 900.0f) return;
    ns = (int)(rad * 9.0f) + 16;
    if (ns > 3600) ns = 3600;
    da = P173_TAU / (float)ns;
    a = 0.0f;
    /* the shorter the arc step the more splats land per pixel: keep the
       deposited energy per unit arc constant so small pearls are not blinding */
    w *= (P173_TAU * rad) / (float)ns;
    for (i = 0; i < ns; i++, a += da) {
        float ca = cosf(a), sa = sinf(a);
        /* core plus a one-pixel halo either side: a cored glow, not a hairline */
        p173_splat(cx + rad * ca, cy + rad * sa, c, w);
        p173_splat(cx + (rad + 1.3f) * ca, cy + (rad + 1.3f) * sa, c, w * 0.40f);
        if (rad > 2.0f)
            p173_splat(cx + (rad - 1.3f) * ca, cy + (rad - 1.3f) * sa, c, w * 0.40f);
    }
}

static void p173_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P173_W * P173_H * 3; i++) {
        int ti = (int)(p173_acc[i] * 256.0f);
        p173_img[i] = p173_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p173_xmw != w) {
        free(p173_xm);
        p173_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p173_xm[x] = (int)(((long long)x * (P173_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p173_xmw = w;
    }
    jd_up_blit(&p173_up, fb, w, h, p173_img, P173_W, P173_H);
}

/* invert circle (cx,cy,rad) in the circle centred (ox,oy) with power k2 */
static int p173_invert(float cx, float cy, float rad, float ox, float oy,
                       float k2, float *ncx, float *ncy, float *nrad)
{
    float dx = cx - ox, dy = cy - oy;
    float den = dx * dx + dy * dy - rad * rad;
    float s;
    if (den < 0.02f) return 0;                    /* centre inside the circle */
    s = k2 / den;
    *ncx = ox + dx * s; *ncy = oy + dy * s; *nrad = rad * s;
    return 1;
}

void pattern_173(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float ox, oy, k2, sc, tx, ty;
    float scale[P173_NCH], rho[P173_NCH];
    float bx, by, br;
    int j, i;
    (void)sl;
    if (!p173_ready || p173_seedc != seed) p173_setup(seed);
    p173_hues(pal);
    memset(p173_acc, 0, sizeof p173_acc);

    {   /* nest the chains: inner boundary of j is the outer boundary of j+1 */
        float s = 1.0f;
        for (j = 0; j < P173_NCH; j++) {
            rho[j] = sinf((float)M_PI / (float)p173_n[j]);
            scale[j] = s;
            s = s * (1.0f - rho[j]) / (1.0f + rho[j]);
        }
    }
    {   /* the inversion centre drifts outside the whole nest */
        float a = t * 0.00043f;
        float rr = p173_orb + 0.30f * sinf(t * 0.00061f);
        ox = rr * cosf(a); oy = rr * sinf(a);
        k2 = p173_pow * (1.0f + 0.10f * sinf(t * 0.00037f));
    }
    /* frame the picture on the image of the outermost boundary circle */
    if (!p173_invert(0.0f, 0.0f, 1.0f + rho[0], ox, oy, k2, &bx, &by, &br)) {
        p173_blit(fb, w, h);
        return;
    }
    sc = (float)P173_H * 0.455f / br;
    tx = (float)P173_W * 0.5f - bx * sc;
    ty = (float)P173_H * 0.5f - by * sc;

    for (j = 0; j < P173_NCH; j++) {
        int n = p173_n[j];
        float s = scale[j], r0 = rho[j] * s;
        float ph = p173_spin * t * (j & 1 ? -1.0f : 1.0f) * (1.0f + 0.35f * (float)j);
        float px = 0.0f, py = 0.0f, pr = 0.0f;
        int have = 0, first = 1;
        float fx = 0.0f, fy = 0.0f, fr = 0.0f;
        /* the two boundary circles, faint */
        {
            float ncx, ncy, nr;
            const float *c = p173_col[(j * 21 + 4) & 63];
            if (p173_invert(0.0f, 0.0f, s * (1.0f + rho[j]), ox, oy, k2, &ncx, &ncy, &nr))
                p173_ring(tx + ncx * sc, ty + ncy * sc, nr * sc, c, 0.30f);
            if (p173_invert(0.0f, 0.0f, s * (1.0f - rho[j]), ox, oy, k2, &ncx, &ncy, &nr))
                p173_ring(tx + ncx * sc, ty + ncy * sc, nr * sc, c, 0.30f);
        }
        for (i = 0; i < n; i++) {
            float a = P173_TAU * (float)i / (float)n + ph;
            float cx = s * cosf(a), cy = s * sinf(a);
            float ncx, ncy, nr, wgt, pulse;
            const float *c;
            if (!p173_invert(cx, cy, r0, ox, oy, k2, &ncx, &ncy, &nr)) { have = 0; continue; }
            ncx = tx + ncx * sc; ncy = ty + ncy * sc; nr *= sc;
            pulse = 0.5f + 0.5f * sinf(a * 2.0f - t * 0.0125f + (float)j * 1.7f);
            wgt = 0.70f + 1.20f * pulse * pulse;
            c = p173_col[(int)(((float)i / (float)n) * 40.0f + (float)j * 12.0f) & 63];
            p173_ring(ncx, ncy, nr, c, wgt);
            /* tangency spark between consecutive pearls */
            if (have) {
                float f = pr / (pr + nr);
                p173_splat(px + (ncx - px) * f, py + (ncy - py) * f, c, 5.5f * wgt);
            }
            if (first) { fx = ncx; fy = ncy; fr = nr; first = 0; }
            px = ncx; py = ncy; pr = nr; have = 1;
        }
        if (have && !first) {                     /* close the necklace       */
            float f = pr / (pr + fr);
            p173_splat(px + (fx - px) * f, py + (fy - py) * f, p173_col[8], 2.6f);
        }
    }
    p173_blit(fb, w, h);
}
