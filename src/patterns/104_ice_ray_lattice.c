/* 104 Ice-Ray Lattice — a Chinese cracked-ice window, alive.
 * The lattice is built the way the joiners built it: take a panel, cut it in
 * two with a straight bar between two randomly chosen edges, and recur on both
 * halves until the pieces are small. Roughly seventy convex cells come out,
 * every one a different irregular polygon, none of them ever crossing — the
 * classic "ice-ray" (bing lie) window. The tree of cuts is fixed once, but the
 * position of every cut slides on its own slow sine, so the whole lattice keeps
 * flexing: cells stretch, shear and swap proportions while the joinery stays
 * intact. Nothing pops, because the topology never changes. Bars are drawn as
 * glowing strokes over black with the hue running along each bar, and the whole
 * panel drifts and turns a few degrees. Line art, ~85% empty. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p104_up;

#define P104_LW 480
#define P104_LH 360
#define P104_MAXV 12
#define P104_NODES 256

typedef struct { float x[P104_MAXV], y[P104_MAXV]; int n; } p104_poly;

static float p104_acc[P104_LW * P104_LH * 3];
static uint8_t p104_img[P104_LW * P104_LH * 3];
static int *p104_xm;
static int p104_xm_w;
static uint8_t p104_tone[2048];
static uint8_t p104_ramp[256][3];
static uint8_t p104_ci[P104_NODES], p104_cj[P104_NODES];
static float p104_fb0[P104_NODES], p104_fb1[P104_NODES];
static float p104_ph0[P104_NODES], p104_ph1[P104_NODES];
static float p104_w0[P104_NODES], p104_w1[P104_NODES];
static int p104_node;
static float p104_t;
static float p104_rc, p104_rs, p104_sc, p104_ox, p104_oy;
static int p104_hbase;
static int p104_ready;

static void p104_init(void)
{
    uint32_t r = 0x104ACE01u ^ 0x9E3779B9u;
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 1.0f - expf(-(float)i * (4.2f / 2048.0f));
        p104_tone[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    for (i = 0; i < P104_NODES; i++) {
        r = r * 1664525u + 1013904223u; p104_ci[i] = (uint8_t)(r >> 16);
        r = r * 1664525u + 1013904223u; p104_cj[i] = (uint8_t)(r >> 16);
        r = r * 1664525u + 1013904223u; p104_fb0[i] = 0.34f + (float)(r >> 20 & 255) * 0.00125f;
        r = r * 1664525u + 1013904223u; p104_fb1[i] = 0.34f + (float)(r >> 20 & 255) * 0.00125f;
        r = r * 1664525u + 1013904223u; p104_ph0[i] = (float)(r >> 12 & 4095) * 0.001534f;
        r = r * 1664525u + 1013904223u; p104_ph1[i] = (float)(r >> 12 & 4095) * 0.001534f;
        r = r * 1664525u + 1013904223u; p104_w0[i] = 0.0006f + (float)(r >> 20 & 255) * 0.0000075f;
        r = r * 1664525u + 1013904223u; p104_w1[i] = 0.0006f + (float)(r >> 20 & 255) * 0.0000075f;
    }
    p104_ready = 1;
}

static void p104_build_ramp(const uint32_t *pal, int base)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * 128) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p104_ramp[i][0] = p104_ramp[i-1][0];
                     p104_ramp[i][1] = p104_ramp[i-1][1];
                     p104_ramp[i][2] = p104_ramp[i-1][2]; }
            else   { p104_ramp[i][0] = p104_ramp[i][1] = p104_ramp[i][2] = 210; }
            continue;
        }
        p104_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p104_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p104_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static void p104_dot(float x, float y, const uint8_t *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, sk, *p;
    float r = c[0] * wgt, g = c[1] * wgt, b = c[2] * wgt;
    if ((unsigned)(xi - 1) >= P104_LW - 3 || (unsigned)(yi - 1) >= P104_LH - 3) return;
    fx = x - (float)xi; fy = y - (float)yi;
    sk = 0.5f;
    p = p104_acc + (yi * P104_LW + xi) * 3;
    {
        float w00 = (1.0f - fx) * (1.0f - fy), w10 = fx * (1.0f - fy);
        float w01 = (1.0f - fx) * fy, w11 = fx * fy;
        p[0] += r * w00; p[1] += g * w00; p[2] += b * w00;
        p[3] += r * w10; p[4] += g * w10; p[5] += b * w10;
        p[-3] += r * sk; p[-2] += g * sk; p[-1] += b * sk;
        p[6] += r * sk; p[7] += g * sk; p[8] += b * sk;
        p -= P104_LW * 3;
        p[0] += r * sk; p[1] += g * sk; p[2] += b * sk;
        p += P104_LW * 6;
        p[0] += r * w01; p[1] += g * w01; p[2] += b * w01;
        p[3] += r * w11; p[4] += g * w11; p[5] += b * w11;
        p += P104_LW * 3;
        p[0] += r * sk; p[1] += g * sk; p[2] += b * sk;
    }
}

/* one bar of the lattice, in panel coordinates */
static void p104_bar(float ax, float ay, float bx, float by, int hue)
{
    float px0 = p104_ox + (ax * p104_rc - ay * p104_rs) * p104_sc;
    float py0 = p104_oy + (ax * p104_rs + ay * p104_rc) * p104_sc;
    float px1 = p104_ox + (bx * p104_rc - by * p104_rs) * p104_sc;
    float py1 = p104_oy + (bx * p104_rs + by * p104_rc) * p104_sc;
    float dx = px1 - px0, dy = py1 - py0;
    float len = sqrtf(dx * dx + dy * dy);
    int n = (int)(len * 1.3f) + 1, k;
    float inv = 1.0f / (float)n;
    const uint8_t *cp = p104_ramp[hue & 255];
    if (n > 900) n = 900;
    for (k = 0; k <= n; k++) {
        float u = (float)k * inv;
        p104_dot(px0 + dx * u, py0 + dy * u, cp, 0.052f);
    }
}

/* TEMPORAL REVIEW 2.4.0 (docs/review/04_pattern_temporal.md, F-104): the
 * header promises "the tree of cuts is fixed once" — it was not.  Every cut
 * chose "the longest edge" and "the longest non-adjacent edge" of the
 * CURRENT, flexing polygon, and the recursion stopped on the CURRENT area,
 * so whenever two edges swapped rank (or a cell crossed the area floor) the
 * topology of the whole subtree changed in one frame: the lattice restructured
 * itself (measured delta 11.5 on a 0.22 median, ~10 times per 300 frames).
 * The split now carries a REFERENCE polygon R alongside the animated P: R is
 * the same recursion with the cut fractions at their rest values (no sine),
 * and every DECISION — where to stop, which edges to cut, what hue — is made
 * on R, while only the emitted coordinates come from P.  R never moves, so
 * the topology is truly fixed and the cells flex continuously. */
static void p104_split(const p104_poly *P, const p104_poly *R, int depth)
{
    p104_poly A, B, RA, RB;
    int i, j, k, m, node = p104_node++ & (P104_NODES - 1);
    float f0, f1, ax, ay, bx, by, rax, ray, rbx, rby, area = 0.0f;

    for (i = 0; i < R->n; i++) {
        int nx = (i + 1) % R->n;
        area += R->x[i] * R->y[nx] - R->x[nx] * R->y[i];
    }
    area = area < 0.0f ? -area * 0.5f : area * 0.5f;

    if (depth == 0 || area < 0.055f || R->n < 3 || R->n > P104_MAXV - 2) {
        for (i = 0; i < P->n; i++) {         /* emit: draw the cell outline */
            int nx = (i + 1) % P->n;
            p104_bar(P->x[i], P->y[i], P->x[nx], P->y[nx],
                     p104_hbase + depth * 21 + (int)(R->x[i] * 47.0f));
        }
        return;
    }
    /* joiner's rule: cut from the longest edge to the longest edge that is not
     * adjacent to it. Random edge picks make slivers; this makes cells with
     * usable proportions, which is what the real lattices look like.
     * Measured on the REST geometry R so the choice never flips. */
    {
        float best = -1.0f, best2 = -1.0f;
        int nb = p104_ci[node] & 1;
        i = 0; j = 2;
        for (k = 0; k < R->n; k++) {
            int nx = (k + 1) % R->n;
            float ex = R->x[nx] - R->x[k], ey = R->y[nx] - R->y[k];
            float L = ex * ex + ey * ey;
            if (L > best) { best = L; i = k; }
        }
        for (k = 0; k < R->n; k++) {
            int d = k - i; if (d < 0) d += R->n;
            if (d < 2 || d > R->n - 2) continue;      /* must not be adjacent */
            {
                int nx = (k + 1) % R->n;
                float ex = R->x[nx] - R->x[k], ey = R->y[nx] - R->y[k];
                float L = ex * ex + ey * ey + (nb ? 0.004f * (float)d : 0.0f);
                if (L > best2) { best2 = L; j = k; }
            }
        }
        if (best2 < 0.0f) { for (i = 0; i < P->n; i++) { int nx = (i + 1) % P->n;
                p104_bar(P->x[i], P->y[i], P->x[nx], P->y[nx], p104_hbase); } return; }
    }
    f0 = p104_fb0[node] + 0.13f * sinf(p104_t * p104_w0[node] + p104_ph0[node]);
    f1 = p104_fb1[node] + 0.13f * sinf(p104_t * p104_w1[node] + p104_ph1[node]);
    {
        int i2 = (i + 1) % P->n, j2 = (j + 1) % P->n;
        ax = P->x[i] + (P->x[i2] - P->x[i]) * f0;
        ay = P->y[i] + (P->y[i2] - P->y[i]) * f0;
        bx = P->x[j] + (P->x[j2] - P->x[j]) * f1;
        by = P->y[j] + (P->y[j2] - P->y[j]) * f1;
        rax = R->x[i] + (R->x[i2] - R->x[i]) * p104_fb0[node];
        ray = R->y[i] + (R->y[i2] - R->y[i]) * p104_fb0[node];
        rbx = R->x[j] + (R->x[j2] - R->x[j]) * p104_fb1[node];
        rby = R->y[j] + (R->y[j2] - R->y[j]) * p104_fb1[node];
    }
    /* A = a -> (i+1 .. j) -> b ; B = b -> (j+1 .. i) -> a  (same walk on R) */
    m = 0;
    A.x[m] = ax; A.y[m] = ay; RA.x[m] = rax; RA.y[m] = ray; m++;
    k = (i + 1) % P->n;
    for (;;) {
        A.x[m] = P->x[k]; A.y[m] = P->y[k]; RA.x[m] = R->x[k]; RA.y[m] = R->y[k]; m++;
        if (k == j || m >= P104_MAXV - 1) break;
        k = (k + 1) % P->n;
    }
    A.x[m] = bx; A.y[m] = by; RA.x[m] = rbx; RA.y[m] = rby; m++;
    A.n = RA.n = m;

    m = 0;
    B.x[m] = bx; B.y[m] = by; RB.x[m] = rbx; RB.y[m] = rby; m++;
    k = (j + 1) % P->n;
    for (;;) {
        B.x[m] = P->x[k]; B.y[m] = P->y[k]; RB.x[m] = R->x[k]; RB.y[m] = R->y[k]; m++;
        if (k == i || m >= P104_MAXV - 1) break;
        k = (k + 1) % P->n;
    }
    B.x[m] = ax; B.y[m] = ay; RB.x[m] = rax; RB.y[m] = ray; m++;
    B.n = RB.n = m;

    p104_split(&A, &RA, depth - 1);
    p104_split(&B, &RB, depth - 1);
}

static void p104_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p104_xm_w != w) {
        free(p104_xm);
        p104_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p104_xm[x] = (int)(((long long)x * (P104_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p104_xm_w = w;
    }
    jd_up_blit(&p104_up, fb, w, h, p104_img, P104_LW, P104_LH);
}

void pattern_104(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    p104_poly root;
    int i, n3 = P104_LW * P104_LH * 3;
    (void)sl;

    if (!p104_ready) p104_init();
    p104_t = t;
    p104_hbase = ((int)(t * 1.3f) + (int)(seed & 32767)) / 9;
    p104_build_ramp(pal, (int)(t * 1.3f) + (int)(seed & 32767));
    {
        float ph = (float)(seed & 4095) * 0.00153f;
        float ang = 0.11f * sinf(t * 0.00037f + ph);
        p104_rc = cosf(ang); p104_rs = sinf(ang);
        p104_sc = 168.0f * (1.0f + 0.045f * sinf(t * 0.00049f + ph * 1.7f));
        p104_ox = P104_LW * 0.5f + 6.0f * sinf(t * 0.00031f);
        p104_oy = P104_LH * 0.5f + 5.0f * sinf(t * 0.00027f + 1.7f);
    }

    root.n = 6;                                   /* a hexagonal panel */
    for (i = 0; i < 6; i++) {
        float a = (float)i * 1.0471976f + 0.5235988f;
        root.x[i] = 1.34f * cosf(a);
        root.y[i] = 1.02f * sinf(a);
    }
    memset(p104_acc, 0, sizeof p104_acc);
    p104_node = (int)(seed & 63);
    p104_split(&root, &root, 8);

    for (i = 0; i < n3; i++) {
        int ti = (int)(p104_acc[i] * 6.0f);
        if (ti > 2047) ti = 2047;
        p104_img[i] = p104_tone[ti];
    }
    p104_blit(fb, w, h);
}
