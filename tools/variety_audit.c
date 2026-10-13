/* variety_audit.c — measure every pattern for the three ways a routine makes
 * JellyDazzle feel repetitive (J, 2026-10-12: "I see this same shape over and
 * over… no new colors"). Headless; links the same objects as gate_harness.
 *
 *   SEED-DEAF     the same picture whatever seed it is spawned with, so every
 *                 appearance looks identical (099 racetrack, 058 fountain).
 *   PALETTE-DEAF  the colours barely follow the palette, so C does not
 *                 change it and it always wears the same colours.
 *   STILL         almost no motion once drawn.
 *
 * For each pattern, three runs of WARM frames each, same frame numbers:
 *   A = seed 1, scheme SA      B = seed 2, scheme SA      C = seed 1, scheme SB
 * dSeed = mean |A-B|, dPal = mean chroma difference A vs C (brightness
 * normalised out, so a palette that only changes how bright it is does not
 * count), motion = mean |A(f) - A(f-30)|. Output: CSV on stdout.
 *
 *   variety_audit W H [first last]        pattern indices, default all */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../src/engine/jellydazzle.h"

extern const uint32_t jd_palette[];          /* JD_NS x 32768, in draw.s */
#define PAL_N 32768
#define WARM 90
#define F0   1000

static double absdiff(const uint32_t *a, const uint32_t *b, int n)
{
    uint64_t s = 0;
    for (int i = 0; i < n; i++) {
        uint32_t p = a[i], q = b[i];
        s += (uint64_t)abs((int)(p >> 16 & 255) - (int)(q >> 16 & 255))
           + (uint64_t)abs((int)(p >> 8 & 255) - (int)(q >> 8 & 255))
           + (uint64_t)abs((int)(p & 255) - (int)(q & 255));
    }
    return (double)s / (3.0 * n);
}

/* colour difference with brightness taken out: compare (r,g,b)/max per pixel,
 * skipping near-black pixels where hue is noise. 0..255 scale. */
static double chromadiff(const uint32_t *a, const uint32_t *b, int n)
{
    double s = 0; int m = 0;
    for (int i = 0; i < n; i += 3) {
        uint32_t p = a[i], q = b[i];
        int pr = p >> 16 & 255, pg = p >> 8 & 255, pb = p & 255;
        int qr = q >> 16 & 255, qg = q >> 8 & 255, qb = q & 255;
        int pm = pr > pg ? (pr > pb ? pr : pb) : (pg > pb ? pg : pb);
        int qm = qr > qg ? (qr > qb ? qr : qb) : (qg > qb ? qg : qb);
        if (pm < 24 || qm < 24) continue;
        s += (fabs((double)pr / pm - (double)qr / qm) + fabs((double)pg / pm - (double)qg / qm)
            + fabs((double)pb / pm - (double)qb / qm)) * 255.0 / 3.0;
        m++;
    }
    return m ? s / m : 0.0;
}

static double luma(const uint32_t *a, int n)
{
    double s = 0;
    for (int i = 0; i < n; i++)
        s += ((a[i] >> 16 & 255) * 54 + (a[i] >> 8 & 255) * 183 + (a[i] & 255) * 19) / 256.0;
    return s / n;
}

/* run one condition; keep the last frame and the frame 30 before it */
static void run(jd_pattern_fn fn, uint32_t *fb, uint32_t *last, uint32_t *early,
                int w, int h, uint32_t seed, const uint32_t *pal)
{
    int n = w * h;
    for (int f = 0; f < WARM; f++) {
        fn(fb, w, h, F0 + f, f, seed, pal);
        if (f == WARM - 31) memcpy(early, fb, (size_t)n * 4);
    }
    memcpy(last, fb, (size_t)n * 4);
}

int main(int argc, char **argv)
{
    int w = argc > 2 ? atoi(argv[1]) : 320, h = argc > 2 ? atoi(argv[2]) : 240;
    int first = argc > 4 ? atoi(argv[3]) : 0;
    int last  = argc > 4 ? atoi(argv[4]) : jd_pattern_count - 1;
    if (last >= jd_pattern_count) last = jd_pattern_count - 1;
    if (first < 0) first = 0;
    if (first > last) { fprintf(stderr, "variety_audit: pattern range must be 0..%d\n",
                                jd_pattern_count - 1); return 2; }
    int n = w * h;
    uint32_t *fb = malloc((size_t)n * 4), *A = malloc((size_t)n * 4), *Ae = malloc((size_t)n * 4);
    uint32_t *B = malloc((size_t)n * 4), *C = malloc((size_t)n * 4), *tmp = malloc((size_t)n * 4);
    /* two schemes far apart in the library */
    const uint32_t *pa = jd_palette + (size_t)7 * PAL_N, *pb = jd_palette + (size_t)97 * PAL_N;
    printf("idx,name,family,luma,motion,dSeed,dPal,seedRatio\n");
    for (int i = first; i <= last; i++) {
        jd_pattern_fn fn = jd_patterns[i];
        run(fn, fb, A, Ae, w, h, 0x1111u, pa);
        run(fn, fb, B, tmp, w, h, 0x9E3779B9u, pa);
        run(fn, fb, C, tmp, w, h, 0x1111u, pb);
        double mo = absdiff(A, Ae, n), ds = absdiff(A, B, n), dp = chromadiff(A, C, n);
        /* seedRatio: how much more two seeds differ than the pattern differs
         * from itself 30 frames later. Seed-deaf patterns sit near or below 1. */
        double sr = ds / (mo > 0.5 ? mo : 0.5);
        printf("%d,\"%s\",%d,%.1f,%.2f,%.2f,%.2f,%.2f\n", i, jd_pattern_names[i],
               jd_pattern_family[i], luma(A, n), mo, ds, dp, sr);
        fflush(stdout);
    }
    return 0;
}
