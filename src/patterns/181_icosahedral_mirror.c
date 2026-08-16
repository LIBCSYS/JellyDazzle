/* 181 Icosahedral Mirror — a kaleidoscope built on the sphere instead of the
 * plane. Every screen pixel is lifted onto the unit sphere by the inverse
 * stereographic map  P = (2x, 2y, q-1)/(q+1),  q = x*x+y*y, then measured
 * against the fifteen mirror planes of the icosahedral group Ih — the three
 * coordinate axes plus the twelve (+-1, +-1/phi, +-phi)/2 and cyclic, i.e. the
 * normals of its fifteen two-fold axes. Distance to the nearest plane draws a
 * Lorentzian filament, distance to the SECOND nearest lights the nodes where
 * great circles cross, so the 120-triangle disdyakis web appears as glowing
 * wire with jewelled 3-, 5- and 2-fold junctions. Line width is scaled by the
 * stereographic Jacobian 2/(1+q) so filaments keep a constant screen thickness
 * from the pole out to the rim. The sphere turns under three detuned slow
 * rotations, and a travelling spherical wave walks hue along the wire while a
 * front-facing term dims the far hemisphere into the black. Pure line art on
 * black — roughly 90% of the frame is near-zero, so it MAX-composites straight
 * over any ground. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p181_up;

#define CW 480
#define CH 360
#define NPL 15

static unsigned char p181_img[CW * CH * 3];
static float p181_bx[CW * CH], p181_by[CW * CH], p181_bz[CW * CH];
static float p181_wsq[CW * CH], p181_vig[CW * CH];
static float p181_nx[NPL], p181_ny[NPL], p181_nz[NPL];
static int *p181_xm;
static int p181_xmw;
static float p181_hue0, p181_huew, p181_r0, p181_r1, p181_r2, p181_kw;
static uint32_t p181_seedc;
static int p181_ready, p181_tabs;

static uint32_t p181_rs;
static float p181_rf(void)
{
    p181_rs ^= p181_rs << 13; p181_rs ^= p181_rs >> 17; p181_rs ^= p181_rs << 5;
    return (float)(p181_rs >> 8) * (1.0f / 16777216.0f);
}

static void p181_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static void p181_tables(void)
{
    const float phi = 1.6180339887f, ip = 0.6180339887f;
    float v[3];
    int i, c, s, x, y, n = 0;
    p181_nx[n] = 1.0f; p181_ny[n] = 0.0f; p181_nz[n] = 0.0f; n++;
    p181_nx[n] = 0.0f; p181_ny[n] = 1.0f; p181_nz[n] = 0.0f; n++;
    p181_nx[n] = 0.0f; p181_ny[n] = 0.0f; p181_nz[n] = 1.0f; n++;
    for (c = 0; c < 3; c++)
        for (s = 0; s < 4; s++) {
            float a = 0.5f, b = 0.5f * ip * ((s & 1) ? -1.0f : 1.0f);
            float d = 0.5f * phi * ((s & 2) ? -1.0f : 1.0f);
            v[c] = a; v[(c + 1) % 3] = b; v[(c + 2) % 3] = d;
            p181_nx[n] = v[0]; p181_ny[n] = v[1]; p181_nz[n] = v[2]; n++;
        }
    for (y = 0; y < CH; y++)
        for (x = 0; x < CW; x++) {
            float px = ((float)x + 0.5f - CW * 0.5f) * (1.0f / 148.0f);
            float py = ((float)y + 0.5f - CH * 0.5f) * (1.0f / 148.0f);
            float q = px * px + py * py, id = 1.0f / (q + 1.0f), wj;
            int o = y * CW + x;
            p181_bx[o] = 2.0f * px * id;
            p181_by[o] = 2.0f * py * id;
            p181_bz[o] = (q - 1.0f) * id;
            wj = 0.0235f * 2.0f * id;                  /* constant screen width */
            if (wj < 0.0009f) wj = 0.0009f;
            p181_wsq[o] = wj * wj;
            p181_vig[o] = 1.0f / (1.0f + 0.055f * q); /* rim falls off gently  */
        }
    (void)i;
    p181_tabs = 1;
}

static void p181_setup(uint32_t seed)
{
    p181_rs = seed ? seed ^ 0x1C0A17u : 0x1C0A17u;
    p181_rf(); p181_rf();
    if (!p181_tabs) p181_tables();
    p181_hue0 = p181_rf();
    p181_huew = 0.10f + p181_rf() * 0.42f;
    p181_r0 = (p181_rf() - 0.5f) * 0.0042f + 0.0016f;
    p181_r1 = (p181_rf() - 0.5f) * 0.0042f - 0.0013f;
    p181_r2 = (p181_rf() - 0.5f) * 0.0031f;
    p181_kw = 2.0f + p181_rf() * 4.0f;
    p181_ready = 1;
    p181_seedc = seed;
}

static void p181_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p181_xmw != w) {
        free(p181_xm);
        p181_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p181_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p181_xmw = w;
    }
    jd_up_blit(&p181_up, fb, w, h, p181_img, CW, CH);
}

void pattern_181(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, m[9], ca, sa, cb, sb, cc, sc, a, b, cnode;
    float colA[3], colB[3];
    int i, o, k;
    (void)sl;
    if (!p181_ready || p181_seedc != seed) p181_setup(seed);
    p181_pal3(pal, p181_hue0, 0.92f, colA);
    p181_pal3(pal, p181_hue0 + p181_huew, 0.80f, colB);

    a = t * p181_r0; b = t * p181_r1;
    ca = cosf(a); sa = sinf(a);
    cb = cosf(b); sb = sinf(b);
    cc = cosf(t * p181_r2); sc = sinf(t * p181_r2);
    /* Rz(c) * Ry(b) * Rx(a) applied to the pixel's sphere point */
    m[0] = cc * cb;
    m[1] = cc * sb * sa - sc * ca;
    m[2] = cc * sb * ca + sc * sa;
    m[3] = sc * cb;
    m[4] = sc * sb * sa + cc * ca;
    m[5] = sc * sb * ca - cc * sa;
    m[6] = -sb;
    m[7] = cb * sa;
    m[8] = cb * ca;
    cnode = 0.62f + 0.30f * sinf(t * 0.0021f);

    for (o = 0; o < CW * CH; o++) {
        float bx = p181_bx[o], by = p181_by[o], bz = p181_bz[o];
        float px = m[0] * bx + m[1] * by + m[2] * bz;
        float py = m[3] * bx + m[4] * by + m[5] * bz;
        float pz = m[6] * bx + m[7] * by + m[8] * bz;
        float d0 = 1e9f, d1 = 1e9f, ws = p181_wsq[o];
        float g, gn, val, hue, fr, col[3];
        unsigned char *dp = p181_img + o * 3;
        for (k = 0; k < NPL; k++) {
            float d = px * p181_nx[k] + py * p181_ny[k] + pz * p181_nz[k];
            d *= d;
            if (d < d0) { d1 = d0; d0 = d; }
            else if (d < d1) d1 = d;
        }
        g  = ws / (ws + d0);
        gn = ws / (ws + d1 * 0.30f);
        val = g * (0.55f + 1.55f * gn * cnode);
        /* front hemisphere reads brighter; rim vignette keeps edges calm */
        fr = 0.30f + 0.70f * (0.5f + 0.5f * pz);
        val *= fr * p181_vig[o];
        hue = 0.5f + 0.5f * sinf(p181_kw * pz + 1.7f * px + t * 0.0037f);
        val = val * val * (1.35f + 0.65f * hue);
        if (val > 1.6f) val = 1.6f;
        for (i = 0; i < 3; i++) {
            float cv = colA[i] + (colB[i] - colA[i]) * hue;
            float ov = (cv * val + 0.14f * val * val * val) * 214.0f + 0.5f;
            col[i] = ov <= 0.0f ? 0.0f : ov >= 255.0f ? 255.0f : ov;
        }
        dp[0] = (unsigned char)col[0];
        dp[1] = (unsigned char)col[1];
        dp[2] = (unsigned char)col[2];
    }
    p181_blit(fb, w, h);
}
