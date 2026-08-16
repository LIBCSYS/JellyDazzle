/* 107 Stereographic Cage — the mirror planes of an icosahedron, projected.
 * The icosahedral group has fifteen two-fold axes; the great circles normal to
 * them cut the sphere into the 120 triangles of the full symmetry group. This
 * routine tumbles that sphere slowly in 3-D and projects it stereographically
 * from the north pole, which maps every great circle to a circle or a straight
 * line in the plane and preserves all the angles exactly. The result is a cage
 * of enormous sweeping arcs that swell toward infinity as their circle passes
 * the projection pole and then contract again — the same figure a kaleidoscope
 * makes, but drawn from the sphere instead of the plane, so the arcs curve.
 * Twelve vertex glints ride along. Line art on black at about 5% coverage: the
 * emptiest overlay in the set, and the only one whose marks are metres long. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p107_up;

#define P107_LW 480
#define P107_LH 360
#define P107_NC 15
#define P107_NS 420

static float p107_acc[P107_LW * P107_LH * 3];
static uint8_t p107_img[P107_LW * P107_LH * 3];
static int *p107_xm;
static int p107_xm_w;
static uint8_t p107_tone[2048];
static uint8_t p107_ramp[256][3];
static float p107_n[P107_NC][3];
static float p107_u[P107_NC][3], p107_v[P107_NC][3];
static float p107_vert[12][3];
static int p107_ready;

static void p107_setax(int i, float x, float y, float z)
{
    float l = 1.0f / sqrtf(x * x + y * y + z * z);
    p107_n[i][0] = x * l; p107_n[i][1] = y * l; p107_n[i][2] = z * l;
}

static void p107_init(void)
{
    const float PHI = 1.6180339887f;
    int i, k = 0;
    for (i = 0; i < 2048; i++) {
        float v = 1.0f - expf(-(float)i * (4.4f / 2048.0f));
        p107_tone[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    p107_setax(k++, 1.0f, 0.0f, 0.0f);
    p107_setax(k++, 0.0f, 1.0f, 0.0f);
    p107_setax(k++, 0.0f, 0.0f, 1.0f);
    for (i = 0; i < 4; i++) {           /* (+-1, +-phi, +-1/phi) and cyclic */
        float sa = (i & 1) ? -1.0f : 1.0f;
        float sb = (i & 2) ? -1.0f : 1.0f;
        p107_setax(k++, 1.0f, sa * PHI, sb / PHI);
        p107_setax(k++, sa * PHI, sb / PHI, 1.0f);
        p107_setax(k++, sb / PHI, 1.0f, sa * PHI);
    }
    for (i = 0; i < P107_NC; i++) {     /* an orthonormal frame per circle */
        float *n = p107_n[i], ax[3], l;
        if (n[0] * n[0] < 0.6f) { ax[0] = 1.0f; ax[1] = 0.0f; ax[2] = 0.0f; }
        else                    { ax[0] = 0.0f; ax[1] = 1.0f; ax[2] = 0.0f; }
        p107_u[i][0] = n[1] * ax[2] - n[2] * ax[1];
        p107_u[i][1] = n[2] * ax[0] - n[0] * ax[2];
        p107_u[i][2] = n[0] * ax[1] - n[1] * ax[0];
        l = 1.0f / sqrtf(p107_u[i][0] * p107_u[i][0] + p107_u[i][1] * p107_u[i][1]
                       + p107_u[i][2] * p107_u[i][2]);
        p107_u[i][0] *= l; p107_u[i][1] *= l; p107_u[i][2] *= l;
        p107_v[i][0] = n[1] * p107_u[i][2] - n[2] * p107_u[i][1];
        p107_v[i][1] = n[2] * p107_u[i][0] - n[0] * p107_u[i][2];
        p107_v[i][2] = n[0] * p107_u[i][1] - n[1] * p107_u[i][0];
    }
    k = 0;
    for (i = 0; i < 4; i++) {           /* icosahedron vertices */
        float sa = (i & 1) ? -1.0f : 1.0f;
        float sb = (i & 2) ? -1.0f : 1.0f;
        float l = 1.0f / sqrtf(1.0f + PHI * PHI);
        p107_vert[k][0] = 0.0f;     p107_vert[k][1] = sa * l;       p107_vert[k][2] = sb * PHI * l; k++;
        p107_vert[k][0] = sa * l;   p107_vert[k][1] = sb * PHI * l; p107_vert[k][2] = 0.0f;         k++;
        p107_vert[k][0] = sb * PHI * l; p107_vert[k][1] = 0.0f;     p107_vert[k][2] = sa * l;       k++;
    }
    p107_ready = 1;
}

static void p107_build_ramp(const uint32_t *pal, int base)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * 128) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p107_ramp[i][0] = p107_ramp[i-1][0];
                     p107_ramp[i][1] = p107_ramp[i-1][1];
                     p107_ramp[i][2] = p107_ramp[i-1][2]; }
            else   { p107_ramp[i][0] = p107_ramp[i][1] = p107_ramp[i][2] = 210; }
            continue;
        }
        p107_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p107_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p107_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static void p107_dot(float x, float y, const uint8_t *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, sk, *p;
    float r = c[0] * wgt, g = c[1] * wgt, b = c[2] * wgt;
    if ((unsigned)(xi - 1) >= P107_LW - 3 || (unsigned)(yi - 1) >= P107_LH - 3) return;
    fx = x - (float)xi; fy = y - (float)yi;
    sk = 0.45f;
    p = p107_acc + (yi * P107_LW + xi) * 3;
    {
        float w00 = (1.0f - fx) * (1.0f - fy), w10 = fx * (1.0f - fy);
        float w01 = (1.0f - fx) * fy, w11 = fx * fy;
        p[0] += r * w00; p[1] += g * w00; p[2] += b * w00;
        p[3] += r * w10; p[4] += g * w10; p[5] += b * w10;
        p[-3] += r * sk; p[-2] += g * sk; p[-1] += b * sk;
        p[6] += r * sk; p[7] += g * sk; p[8] += b * sk;
        p -= P107_LW * 3;
        p[0] += r * sk; p[1] += g * sk; p[2] += b * sk;
        p += P107_LW * 6;
        p[0] += r * w01; p[1] += g * w01; p[2] += b * w01;
        p[3] += r * w11; p[4] += g * w11; p[5] += b * w11;
        p += P107_LW * 3;
        p[0] += r * sk; p[1] += g * sk; p[2] += b * sk;
    }
}

static void p107_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p107_xm_w != w) {
        free(p107_xm);
        p107_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p107_xm[x] = (int)(((long long)x * (P107_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p107_xm_w = w;
    }
    jd_up_blit(&p107_up, fb, w, h, p107_img, P107_LW, P107_LH);
}

void pattern_107(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float ca, sa, cb, sb, cg, sg, sc, ox, oy;
    int i, j, n3 = P107_LW * P107_LH * 3, hbase;
    (void)sl;

    if (!p107_ready) p107_init();
    hbase = (int)(t * 1.3f) + (int)(seed & 32767);
    p107_build_ramp(pal, hbase);
    {
        float ph = (float)(seed & 4095) * 0.00153f;
        float a = t * 0.00121f + ph, b = t * 0.00083f + ph * 1.7f;
        float g = t * 0.00047f + ph * 0.6f;
        ca = cosf(a); sa = sinf(a);
        cb = cosf(b); sb = sinf(b);
        cg = cosf(g); sg = sinf(g);
        sc = 96.0f * (1.0f + 0.07f * sinf(t * 0.00041f));
        ox = P107_LW * 0.5f; oy = P107_LH * 0.5f;
    }
    memset(p107_acc, 0, sizeof p107_acc);

#define P107_ROT(X, Y, Z, RX, RY, RZ) do {          \
        float t1x = (X) * ca - (Y) * sa;            \
        float t1y = (X) * sa + (Y) * ca;            \
        float t2y = t1y * cb - (Z) * sb;            \
        float t2z = t1y * sb + (Z) * cb;            \
        (RX) = t1x * cg - t2z * sg;                 \
        (RZ) = t1x * sg + t2z * cg;                 \
        (RY) = t2y;                                 \
    } while (0)

    for (i = 0; i < P107_NC; i++) {
        const uint8_t *cp = p107_ramp[(hbase / 14 + i * 17) & 255];
        float px = 0.0f, py = 0.0f;
        int have = 0;
        for (j = 0; j <= P107_NS; j++) {
            float a = (float)j * (6.2831853f / (float)P107_NS);
            float c = cosf(a), s = sinf(a);
            float wx = p107_u[i][0] * c + p107_v[i][0] * s;
            float wy = p107_u[i][1] * c + p107_v[i][1] * s;
            float wz = p107_u[i][2] * c + p107_v[i][2] * s;
            float rx, ry, rz, d, qx, qy;
            P107_ROT(wx, wy, wz, rx, ry, rz);
            d = 1.0f - rz;
            if (d < 0.035f) { have = 0; continue; }   /* past the pole */
            qx = ox + rx / d * sc;
            qy = oy + ry / d * sc;
            if (qx < -900.0f || qx > P107_LW + 900.0f ||
                qy < -900.0f || qy > P107_LH + 900.0f) { have = 0; continue; }
            if (have) {
                float dx = qx - px, dy = qy - py;
                float len = sqrtf(dx * dx + dy * dy);
                int ns = (int)(len * 1.25f) + 1, k;
                float inv, wg;
                if (ns > 400) ns = 400;
                inv = 1.0f / (float)ns;
                wg = 0.150f * inv * (len < 1.0f ? len : 1.0f);
                for (k = 0; k < ns; k++)
                    p107_dot(px + dx * (float)k * inv, py + dy * (float)k * inv,
                             cp, wg);
            }
            px = qx; py = qy; have = 1;
        }
    }
    for (i = 0; i < 12; i++) {                        /* vertex glints */
        float rx, ry, rz, d;
        const uint8_t *cp = p107_ramp[(hbase / 14 + i * 21 + 90) & 255];
        P107_ROT(p107_vert[i][0], p107_vert[i][1], p107_vert[i][2], rx, ry, rz);
        d = 1.0f - rz;
        if (d < 0.06f) continue;
        {
            float qx = ox + rx / d * sc, qy = oy + ry / d * sc;
            float g = 1.5f / (0.6f + d);
            int a2;
            p107_dot(qx, qy, cp, g);
            for (a2 = 0; a2 < 8; a2++) {
                float an = (float)a2 * 0.7853982f;
                p107_dot(qx + cosf(an) * 1.7f, qy + sinf(an) * 1.7f, cp, g * 0.45f);
                p107_dot(qx + cosf(an) * 3.2f, qy + sinf(an) * 3.2f, cp, g * 0.16f);
            }
        }
    }
#undef P107_ROT

    for (i = 0; i < n3; i++) {
        int ti = (int)(p107_acc[i] * 7.0f);
        if (ti > 2047) ti = 2047;
        p107_img[i] = p107_tone[ti];
    }
    p107_blit(fb, w, h);
}
