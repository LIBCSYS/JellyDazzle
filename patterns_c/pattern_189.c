/* 189 Reiter Crystal — a snowflake grown by the real rule, not drawn. Reiter's
 * 2005 hexagonal automaton is run on a 209x189 triangular lattice: a cell is
 * RECEPTIVE if it or any of its six neighbours already holds one unit of ice;
 * receptive cells lock their water away and gain a constant vapour influx
 * gamma, while the rest diffuse with u += (alpha/2)(mean6(u) - u). That single
 * rule, from one seed cell in a uniform vapour field beta, produces genuine
 * snow-crystal morphology — and genuine six-fold symmetry, because the lattice
 * and the seed have it and the rule is local. Nudging beta and gamma per
 * segment walks the crystal across Nakaya's diagram: fat hexagonal plates,
 * sectored stars, or fully branched stellar dendrites. Freezing time is stored
 * per cell and drives hue, so the finished flake carries growth rings like a
 * tree. A 12-frame display lag makes every cell's freeze a fade-in rather than
 * a flip. Accumulator: pre-rolls 100 CA steps at reset so sl == 0 opens on an
 * established flake, then grows all segment and holds and turns. The vapour
 * field itself is drawn as a dim vignetted ground, so the crystal sits in the
 * medium it is eating rather than on a void — the dark aura around it is the
 * real depletion zone. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p189_up;

#define CW 480
#define CH 360
#define GW 209
#define GH 189
#define GCX 104
#define GCY 94
#define RMAX 84.0f

static unsigned char p189_img[CW * CH * 3];
static float p189_s[GW * GH], p189_u[GW * GH], p189_v[GW * GH];
static float p189_d[GW * GH], p189_a[GW * GH];
static int *p189_xm;
static int p189_xmw;
static float p189_alpha, p189_beta, p189_gamma, p189_hue0, p189_huew, p189_rot;
static int p189_step, p189_grow;
static uint32_t p189_seedc;
static int p189_ready, p189_last;

/* --- vapour ground (v2.1) ------------------------------------------------
 * The crystal used to be drawn on absolute 0,0,0: at sl==0 the whole frame
 * measured luma 0.00 / coverage 0.000, and it stayed under luma 1.5 for the
 * first five seconds of every segment.  That is the pure-black sample.  The
 * fix is not a fake backdrop — the supersaturated vapour field p189_s is
 * already simulated, it was simply never drawn.  Rendering it dim gives an
 * intentional cold ground AND shows the depletion halo the crystal eats
 * around itself, which is the physics.  P189_VIG is a static vignette and
 * P189_SIN a separable low-frequency undulation so the far field is calm but
 * not a dead flat wall; both are table lookups, no per-pixel transcendentals. */
#define P189_PRE   100        /* CA steps run at reset: sl==0 opens grown   */
#define P189_VGAIN 30.0f      /* vapour brightness scale                    */
#define P189_UAMP  0.20f      /* undulation depth                           */
static unsigned char p189_vig[CW * CH];
static signed char   p189_sin[512];
static int           p189_tabs = 0;

static void p189_mktabs(void)
{
    int x, y;
    for (x = 0; x < 512; x++)
        p189_sin[x] = (signed char)lrintf(sinf((float)x * (6.2831853f / 512.0f)) * 127.0f);
    for (y = 0; y < CH; y++)
        for (x = 0; x < CW; x++) {
            float dx = ((float)x - CW * 0.5f) / (CW * 0.5f);
            float dy = ((float)y - CH * 0.5f) / (CH * 0.5f);
            float r  = dx * dx + dy * dy;
            float v  = 1.0f - 0.42f * r;
            if (v < 0.10f) v = 0.10f;
            p189_vig[y * CW + x] = (unsigned char)lrintf(v * 255.0f);
        }
    p189_tabs = 1;
}

static uint32_t p189_rs;
static float p189_rf(void)
{
    p189_rs ^= p189_rs << 13; p189_rs ^= p189_rs >> 17; p189_rs ^= p189_rs << 5;
    return (float)(p189_rs >> 8) * (1.0f / 16777216.0f);
}

static void p189_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static void p189_ca(void);

static void p189_reset(uint32_t seed)
{
    int i;
    p189_rs = seed ? seed ^ 0x51A0E7u : 0x51A0E7u;
    p189_rf(); p189_rf();
    p189_alpha = 1.00f + p189_rf() * 0.46f;
    p189_beta  = 0.42f + p189_rf() * 0.42f;
    p189_gamma = 0.0006f + p189_rf() * 0.0042f;
    p189_hue0  = p189_rf();
    p189_huew  = 0.22f + p189_rf() * 0.56f;
    p189_rot   = (p189_rf() < 0.5f ? -1.0f : 1.0f) * (0.00019f + p189_rf() * 0.00028f);
    for (i = 0; i < GW * GH; i++) {
        p189_s[i] = p189_beta;
        p189_d[i] = 0.0f;
        p189_a[i] = 0.0f;
    }
    p189_s[GCY * GW + GCX] = 1.0f;
    p189_step = 0;
    p189_grow = 1;
    p189_ready = 1;
    p189_seedc = seed;
    /* Pre-roll, exactly as pattern_031 primes its accumulator: the segment
     * opens on an established crystal instead of one lit cell.  Measured at
     * 1280x960 the reset frame costs 16.3 ms against 5.5 ms steady — one
     * spawn frame per segment, under bridge.c's own 20 ms SLOW threshold.
     * The flake still has the whole segment left to grow into. */
    for (i = 0; i < P189_PRE; i++) p189_ca();
    /* Settle the display lag so frame 0 is not a fade-up from nothing. */
    for (i = 0; i < GW * GH; i++) {
        float s = p189_s[i], tv = (s - 0.98f) * 1.30f + 0.52f;
        if (s < 1.0f) tv = 0.0f;
        if (tv > 1.0f) tv = 1.0f;
        p189_d[i] = tv;
    }
}

static void p189_ca(void)
{
    int x, y, i, d;
    float ah = p189_alpha * 0.5f, g = p189_gamma, mr = 0.0f;
    for (y = 1; y < GH - 1; y++) {
        d = (y & 1) ? 1 : -1;
        i = y * GW + 1;
        for (x = 1; x < GW - 1; x++, i++) {
            float s = p189_s[i];
            int rec = (s >= 1.0f) ||
                      p189_s[i - 1] >= 1.0f || p189_s[i + 1] >= 1.0f ||
                      p189_s[i - GW] >= 1.0f || p189_s[i + GW] >= 1.0f ||
                      p189_s[i - GW + d] >= 1.0f || p189_s[i + GW + d] >= 1.0f;
            if (rec) { p189_u[i] = 0.0f; p189_v[i] = s + g; }
            else     { p189_u[i] = s;    p189_v[i] = 0.0f; }
        }
    }
    for (y = 0; y < GH; y++) {
        p189_u[y * GW] = p189_beta; p189_u[y * GW + GW - 1] = p189_beta;
        p189_v[y * GW] = 0.0f;      p189_v[y * GW + GW - 1] = 0.0f;
    }
    for (x = 0; x < GW; x++) {
        p189_u[x] = p189_beta; p189_u[(GH - 1) * GW + x] = p189_beta;
        p189_v[x] = 0.0f;      p189_v[(GH - 1) * GW + x] = 0.0f;
    }
    for (y = 1; y < GH - 1; y++) {
        d = (y & 1) ? 1 : -1;
        i = y * GW + 1;
        for (x = 1; x < GW - 1; x++, i++) {
            float u = p189_u[i];
            float m = (p189_u[i - 1] + p189_u[i + 1] + p189_u[i - GW] +
                       p189_u[i + GW] + p189_u[i - GW + d] + p189_u[i + GW + d])
                      * (1.0f / 6.0f);
            float ns = u + ah * (m - u) + p189_v[i];
            p189_s[i] = ns;
            if (ns >= 1.0f && p189_a[i] == 0.0f) {
                float hx = (float)(x - GCX) + 0.5f * (float)((y & 1) - (GCY & 1));
                float hy = (float)(y - GCY) * 0.8660254f;
                float rr = hx * hx + hy * hy;
                p189_a[i] = (float)(p189_step + 1);
                if (rr > mr) mr = rr;
            }
        }
    }
    p189_step++;
    if (mr > RMAX * RMAX) p189_grow = 0;
}

static void p189_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p189_xmw != w) {
        free(p189_xm);
        p189_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p189_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p189_xmw = w;
    }
    jd_up_blit(&p189_up, fb, w, h, p189_img, CW, CH);
}

void pattern_189(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, cs, sn, sc, col[64][3], iage, hph;
    float vcol[3], uph1, uph2;
    int x, y, i, k;
    if (!p189_tabs) p189_mktabs();
    if (!p189_ready || p189_seedc != seed || sl == 0 || sl < p189_last)
        p189_reset(seed);
    p189_last = sl;

    if (p189_grow && (frame & 1) == 0) p189_ca();

    for (i = 0; i < GW * GH; i++) {
        float s = p189_s[i], tv;
        tv = (s - 0.98f) * 1.30f + 0.52f;
        if (s < 1.0f) tv = 0.0f;
        if (tv > 1.0f) tv = 1.0f;
        p189_d[i] += (tv - p189_d[i]) * 0.085f;
    }
    for (k = 0; k < 64; k++)
        p189_pal3(pal, p189_hue0 + p189_huew * ((float)k * (1.0f / 64.0f)), 0.86f, col[k]);
    iage = 1.0f / (float)(p189_step > 40 ? p189_step : 40);

    hph = t * 0.00018f;
    hph -= (float)(int)hph;
    sc = 1.0f / (1.86f + 0.05f * sinf(t * 0.00043f));
    cs = cosf(t * p189_rot) * sc; sn = sinf(t * p189_rot) * sc;
    /* Cold complementary wash for the vapour, desaturated so it stays a
     * ground and never competes with the crystal for the eye. */
    p189_pal3(pal, p189_hue0 + 0.52f, 0.55f, vcol);
    uph1 = t * 0.035f; uph2 = t * -0.026f;

    for (y = 0; y < CH; y++) {
        float dy0 = (float)y + 0.5f - CH * 0.5f;
        float dx0 = 0.5f - CW * 0.5f;
        float hx = dx0 * cs - dy0 * sn, hy = dx0 * sn + dy0 * cs;
        unsigned char *op = p189_img + y * CW * 3;
        const unsigned char *vg = p189_vig + y * CW;
        /* separable undulation: row term fixed, column term is a table read */
        float uy = (float)p189_sin[((int)(uph2 + (float)y * 1.7f)) & 511] * (1.0f / 127.0f);
        float ua = P189_UAMP * uy;
        for (x = 0; x < CW; x++, hx += cs, hy += sn) {
            float fy = hy * (1.15470054f) + (float)GCY, fj, va, vb, ag, br;
            float xi, fi, aa, ab, sa, sb;
            float sv = p189_beta;          /* far-field vapour density      */
            int j0, j1, i0;
            if (fy < 1.0f || fy >= (float)(GH - 2)) goto vapour;
            j0 = (int)fy; fj = fy - (float)j0; j1 = j0 + 1;
            xi = hx + (float)GCX - 0.5f * (float)((j0 & 1) - (GCY & 1));
            if (xi < 1.0f || xi >= (float)(GW - 2)) goto vapour;
            i0 = (int)xi; fi = xi - (float)i0;
            i = j0 * GW + i0;
            va = p189_d[i] + (p189_d[i + 1] - p189_d[i]) * fi;
            aa = p189_a[i] + (p189_a[i + 1] - p189_a[i]) * fi;
            sa = p189_s[i] + (p189_s[i + 1] - p189_s[i]) * fi;
            xi = hx + (float)GCX - 0.5f * (float)((j1 & 1) - (GCY & 1));
            if (xi < 1.0f || xi >= (float)(GW - 2)) goto vapour;
            i0 = (int)xi; fi = xi - (float)i0;
            i = j1 * GW + i0;
            vb = p189_d[i] + (p189_d[i + 1] - p189_d[i]) * fi;
            ab = p189_a[i] + (p189_a[i + 1] - p189_a[i]) * fi;
            sb = p189_s[i] + (p189_s[i + 1] - p189_s[i]) * fi;
            br = va + (vb - va) * fj;
            sv = sa + (sb - sa) * fj;
            if (br <= 0.003f) goto vapour;
            ag = aa + (ab - aa) * fj;
            {
                float hf = ag * iage * 0.92f + hph;
                const float *c = col[(int)((hf - (float)(int)hf) * 63.99f)];
                float e = br * br * (0.42f + 0.58f * br) * 300.0f;
                int r = (int)(c[0] * e), g = (int)(c[1] * e), b = (int)(c[2] * e);
                op[x * 3]     = (unsigned char)(r > 255 ? 255 : r);
                op[x * 3 + 1] = (unsigned char)(g > 255 ? 255 : g);
                op[x * 3 + 2] = (unsigned char)(b > 255 ? 255 : b);
            }
            continue;
        vapour:
            {
                /* Dim, vignetted, gently undulating vapour.  Clamped below
                 * the freezing point so ice never blows the ground out. */
                float d = sv > 1.0f ? 1.0f : sv;
                float u = 1.0f + ua * ((float)p189_sin[((int)(uph1 + (float)x * 1.3f)) & 511]
                                       * (1.0f / 127.0f));
                float e = (d - 0.08f) * P189_VGAIN * u * ((float)vg[x] * (1.0f / 255.0f));
                int r, g, b;
                if (e < 0.0f) e = 0.0f;
                r = (int)(vcol[0] * e); g = (int)(vcol[1] * e); b = (int)(vcol[2] * e);
                op[x * 3]     = (unsigned char)(r > 255 ? 255 : r);
                op[x * 3 + 1] = (unsigned char)(g > 255 ? 255 : g);
                op[x * 3 + 2] = (unsigned char)(b > 255 ? 255 : b);
            }
        }
    }
    p189_blit(fb, w, h);
}
