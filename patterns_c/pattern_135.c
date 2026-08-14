/* 135 Kelvin Wake — the ship-wave pattern, built from its own dispersion.
 *
 * A disturbance moving over deep water leaves a wake whose half-angle is
 * 19.47 degrees no matter how fast it goes -- Kelvin's result, and one of
 * the prettier accidents in physics.  It falls out of the deep-water
 * dispersion: a wave train that keeps station with a hull moving at V, at
 * an angle theta to its track, must have wavenumber k = k0.sec^2(theta).
 * Superposing that one-parameter family is the whole wake, so this pattern
 * sums 120 plane waves with exactly those wavenumbers and nothing else.  The
 * two families the eye reads -- TRANSVERSE crests strung across the track
 * (small theta) and DIVERGENT crests fanning off it (large theta) -- are
 * not drawn separately; they are the two stationary-phase branches of the
 * same sum, and the cusp line where they meet is the 19.47-degree arm.
 *
 * Two hulls run slow circular courses in opposite directions and their
 * wakes cross and interfere.  Everything outside a wedge is skipped
 * entirely, which is both physically right and what makes 80 waves per
 * pixel affordable: the phase of every wave is accumulated incrementally
 * along a row against a cosine table, and a row is only entered where the
 * wedge is live.  Crests are lit by a cubed half-cosine so they read as
 * thin bright lines on dark water rather than a grey ripple field. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AW 320
#define AH 240
#define AN (AW * AH)
#define NWAV 120
#define NSHIP 2
#define LUTN 2048
#define TAN19 0.35355339f          /* tan(19.4712 deg) = 1/(2 sqrt2) */

static float p135_acc[AN * 3];
static float p135_blur[AN * 3];
static uint8_t p135_img[AN * 3];
static uint8_t p135_tone[2048];
static float p135_cos[LUTN];
static float p135_pc[512][3];
static int p135_ready;
static int p135_uw = -1;
static int *p135_uxi;
static uint8_t *p135_ufx;

static void p135_tabs(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (5.0f / 2048.0f)));
        p135_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    for (i = 0; i < LUTN; i++)
        p135_cos[i] = cosf((float)i * (6.2831853f / (float)LUTN));
    p135_ready = 1;
}

#define P135_CS ((float)LUTN * (1.0f / 6.2831853f))
#define P135_COS(ph) p135_cos[((int)((ph) * P135_CS + 1048576.5f)) & (LUTN - 1)]

static void p135_build_col(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 512; i++) {
        uint32_t u = pal[(i << 6) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255);
        float g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float m = r > g ? r : g;
        if (b > m) m = b;
        if (m < 30.0f) m = 30.0f;
        m = 1.0f / m;
        p135_pc[i][0] = 0.10f + 0.90f * r * m;
        p135_pc[i][1] = 0.10f + 0.90f * g * m;
        p135_pc[i][2] = 0.10f + 0.90f * b * m;
    }
}

static void p135_bloom(void)
{
    int x, y, k;
    for (y = 0; y < AH; y++) {
        float *row = p135_acc + (size_t)y * AW * 3;
        float *out = p135_blur + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < AW - 1 ? x + 1 : AW - 1;
            for (k = 0; k < 3; k++)
                out[x * 3 + k] = 0.5f * row[x * 3 + k]
                               + 0.25f * (row[xm * 3 + k] + row[xp * 3 + k]);
        }
    }
    for (y = 0; y < AH; y++) {
        int ym = y > 0 ? y - 1 : 0, yp = y < AH - 1 ? y + 1 : AH - 1;
        const float *a = p135_blur + (size_t)ym * AW * 3;
        const float *b = p135_blur + (size_t)y * AW * 3;
        const float *c = p135_blur + (size_t)yp * AW * 3;
        float *out = p135_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW * 3; x++)
            out[x] = 0.55f * out[x] + 0.45f * (0.5f * b[x] + 0.25f * (a[x] + c[x]));
    }
}

static void p135_resolve(void)
{
    int i;
    for (i = 0; i < AN * 3; i++) {
        int t = (int)(p135_acc[i] * 780.0f);
        if (t < 0) t = 0;
        if (t > 2047) t = 2047;
        p135_img[i] = p135_tone[t];
    }
}

static void p135_upscale(uint32_t *fb, int w, int h)
{
    int x, y, k;
    if (w != p135_uw) {
        free(p135_uxi); free(p135_ufx);
        p135_uxi = (int *)malloc(sizeof(int) * (size_t)w);
        p135_ufx = (uint8_t *)malloc((size_t)w);
        for (x = 0; x < w; x++) {
            long long q = ((long long)x * (AW - 1) * 256) / (w > 1 ? w - 1 : 1);
            int xi = (int)(q >> 8);
            if (xi > AW - 2) { xi = AW - 2; q = (long long)(AW - 1) * 256; }
            p135_uxi[x] = xi * 3;
            p135_ufx[x] = (uint8_t)(q & 255);
        }
        p135_uw = w;
    }
    for (y = 0; y < h; y++) {
        long long qy = ((long long)y * (AH - 1) * 256) / (h > 1 ? h - 1 : 1);
        int yi = (int)(qy >> 8), fy;
        const uint8_t *r0, *r1;
        uint32_t *out;
        if (yi > AH - 2) { yi = AH - 2; qy = (long long)(AH - 1) * 256; }
        fy = (int)(qy & 255);
        r0 = p135_img + (size_t)yi * AW * 3; r1 = r0 + AW * 3;
        out = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int X = p135_uxi[x], fx = p135_ufx[x], c[3];
            for (k = 0; k < 3; k++) {
                int t0 = r0[X + k] + (((r0[X + 3 + k] - r0[X + k]) * fx) >> 8);
                int t1 = r1[X + k] + (((r1[X + 3 + k] - r1[X + k]) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16)
                   | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

void pattern_135(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float sd = (float)(seed & 511) * (1.0f / 512.0f);
    float amp[NWAV], kk[NWAV], ct[NWAV], st[NWAV];
    float shx[NSHIP], shy[NSHIP], shc[NSHIP], shs[NSHIP];
    float k0, dmax, hb;
    int i, x, y, sp;
    (void)sl;

    if (!p135_ready) p135_tabs();
    p135_build_col(pal);
    memset(p135_acc, 0, sizeof p135_acc);

    /* the Kelvin family: k(theta) = k0 sec^2(theta) */
    k0 = 0.375f + 0.045f * sinf(t * 0.00043f + sd * 6.2831f);
    for (i = 0; i < NWAV; i++) {
        float th = (-1.00f + 2.00f * ((float)i + 0.5f) / (float)NWAV);
        float c = cosf(th);
        ct[i] = c; st[i] = sinf(th);
        kk[i] = k0 / (c * c);
        /* taper the ends of the integration range so the truncation of the
         * family does not ring across the wedge */
        /* the exact integral weights theta uniformly; only the truncation
         * of the family is tapered, or the divergent branch disappears */
        {
            float wnd = sinf(((float)i + 0.5f) * (3.14159265f / (float)NWAV));
            amp[i] = wnd * wnd * 0.5f + 0.5f;
        }
    }
    /* the divergent branch has the shortest waves and aliases first; the
     * wake is cut where the phase step between adjacent theta reaches pi */
    dmax = (float)AH * 0.70f;
    hb = t * 0.000031f + sd;

    /* dark water */
    {
        const float *c = p135_pc[(int)(hb * 512.0f) & 511];
        for (y = 0; y < AH; y++) {
            float vy = (float)y * (1.0f / AH) - 0.5f;
            float *row = p135_acc + (size_t)y * AW * 3;
            float g = 0.055f + 0.030f * (0.5f - vy);
            for (x = 0; x < AW; x++) {
                row[x * 3 + 0] += c[0] * g * 0.45f;
                row[x * 3 + 1] += c[1] * g * 0.62f;
                row[x * 3 + 2] += c[2] * g;
            }
        }
    }

    for (sp = 0; sp < NSHIP; sp++) {
        float dir = sp ? -1.0f : 1.0f;
        float orb = t * 0.00068f * dir + (float)sp * 3.4f + sd * 6.2831f;
        float psi = orb + 1.5707963f * dir;      /* heading is tangent */
        float cps, sps, dpx[NWAV];
        int hoff = sp ? 190 : 0;
        shx[sp] = (float)AW * (0.5f + 0.290f * cosf(orb));
        shy[sp] = (float)AH * (0.5f + 0.280f * sinf(orb));
        cps = cosf(psi); sps = sinf(psi);
        shc[sp] = cps; shs[sp] = sps;
        for (i = 0; i < NWAV; i++)
            dpx[i] = kk[i] * (ct[i] * cps - st[i] * sps);

        for (y = 0; y < AH; y++) {
            float dy = (float)y - shy[sp];
            float u0 = (0.0f - shx[sp]) * cps + dy * sps;
            float v0 = -(0.0f - shx[sp]) * sps + dy * cps;
            float ph[NWAV];
            float u = u0, v = v0;
            float *row = p135_acc + (size_t)y * AW * 3;
            int live = 0;
            for (x = 0; x < AW; x++, u += cps, v -= sps) {
                float d = -u, av = fabsf(v), half, edge, env, eta;
                if (d <= 2.0f || d > dmax) { live = 0; continue; }
                half = TAN19 * d;
                if (av >= half) { live = 0; continue; }
                if (!live) {
                    /* entering the wedge on this row: seed every phase once */
                    for (i = 0; i < NWAV; i++)
                        ph[i] = kk[i] * (u * ct[i] + v * st[i]);
                    live = 1;
                } else {
                    for (i = 0; i < NWAV; i++) ph[i] += dpx[i];
                }
                edge = (half - av) * (1.0f / 9.0f);
                if (edge > 1.0f) edge = 1.0f;
                /* real wake amplitude peaks at the cusp arm, not on the track */
                env = edge * (0.55f + 0.75f * (av / half)) / sqrtf(1.0f + d * 0.055f);
                env *= 1.0f - d / dmax;
                if (d < 9.0f) env *= (d - 2.0f) * (1.0f / 7.0f);
                eta = 0.0f;
                for (i = 0; i < NWAV; i++) eta += amp[i] * P135_COS(ph[i]);
                eta *= (1.0f / (float)NWAV) * 4.2f;
                {
                    /* crest lighting: a cubed half-cosine keeps troughs black */
                    float q = 0.5f + 0.5f * eta;
                    float b;
                    const float *c;
                    int ci;
                    if (q < 0.0f) q = 0.0f;
                    if (q > 1.0f) q = 1.0f;
                    b = q * q * q * env * 2.4f;
                    ci = (int)((hb + 0.10f + eta * 0.085f) * 512.0f
                               + (float)hoff) ;
                    c = p135_pc[ci & 511];
                    row[x * 3 + 0] += c[0] * b;
                    row[x * 3 + 1] += c[1] * b;
                    row[x * 3 + 2] += c[2] * b;
                }
            }
        }
        /* hull glint at the apex of the wedge */
        {
            int cx = (int)shx[sp], cy = (int)shy[sp], dx, dy2;
            const float *c = p135_pc[(int)((hb + 0.5f) * 512.0f) & 511];
            for (dy2 = -2; dy2 <= 2; dy2++)
                for (dx = -2; dx <= 2; dx++) {
                    int px = cx + dx, py = cy + dy2;
                    float g = 1.1f / (1.0f + (float)(dx * dx + dy2 * dy2) * 1.3f);
                    if ((unsigned)px >= AW || (unsigned)py >= AH) continue;
                    {
                        float *p = p135_acc + ((size_t)py * AW + px) * 3;
                        p[0] += c[0] * g; p[1] += c[1] * g; p[2] += c[2] * g;
                    }
                }
        }
    }

    p135_bloom();
    p135_resolve();
    p135_upscale(fb, w, h);
}
