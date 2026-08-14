/* qc_harness.c — single-pattern render/measure with a selectable palette scheme.
 *   clang -O2 -DPATTERN=pattern_NNN qc_harness.c ../pattern_NNN.c -o t -lm
 *   JD_SCHEME=k ./t out.ppm START COUNT     (render; "-" = no file)
 * Prints: mean luma, sigma, coverage(frac of px with luma>16), mean/peak
 * frame-to-frame channel delta over the last 60 frames.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "jellydazzle.h"
#define W 1280
#define H 960
void PATTERN(uint32_t*, int, int, int, int, uint32_t, const uint32_t*);
static uint32_t fb[W*H], prev[W*H], pal[32768];
int main(int argc, char **argv)
{
    int scheme = getenv("JD_SCHEME") ? atoi(getenv("JD_SCHEME")) : 0;
    FILE *f = fopen("palette.bin", "rb");
    if (!f) { fprintf(stderr, "no palette.bin\n"); return 1; }
    fseek(f, (long)scheme * 32768L * 4L, SEEK_SET);
    if (fread(pal, 4, 32768, f) != 32768) { fprintf(stderr, "short palette\n"); return 1; }
    fclose(f);
    int start = argc > 2 ? atoi(argv[2]) : 0, count = argc > 3 ? atoi(argv[3]) : 400;
    double dsum = 0, dmax = 0; int dn = 0;
    for (int fr = start; fr < start + count; fr++) {
        memcpy(prev, fb, sizeof fb);
        PATTERN(fb, W, H, fr, fr & 2047, 0xC0FFEE11u ^ (uint32_t)(fr >> 11), pal);
        if (fr >= start + count - 60 && fr > start) {
            uint64_t s = 0;
            for (int i = 0; i < W*H; i += 3) {
                uint32_t c = fb[i], p = prev[i];
                int dr = (int)((c>>16)&255) - (int)((p>>16)&255);
                int dg = (int)((c>>8)&255)  - (int)((p>>8)&255);
                int db = (int)(c&255)       - (int)(p&255);
                s += (dr<0?-dr:dr) + (dg<0?-dg:dg) + (db<0?-db:db);
            }
            double d = (double)s / ((W*H/3)*3.0);
            dsum += d; dn++; if (d > dmax) dmax = d;
        }
    }
    double ls = 0, l2 = 0; long cov = 0; int n = 0;
    for (int i = 0; i < W*H; i += 5) {
        uint32_t c = fb[i];
        double l = (((c>>16)&255)*77 + ((c>>8)&255)*150 + (c&255)*29)/256.0;
        ls += l; l2 += l*l; if (l > 16) cov++; n++;
    }
    double m = ls/n, sd = l2/n - m*m; sd = sd > 0 ? __builtin_sqrt(sd) : 0;
    printf("luma %6.2f  sigma %6.2f  cover %.3f  delta mean %6.2f peak %6.2f\n",
           m, sd, (double)cov/n, dsum/(dn?dn:1), dmax);
    if (argc > 1 && strcmp(argv[1], "-")) {
        FILE *o = fopen(argv[1], "wb");
        fprintf(o, "P6\n%d %d\n255\n", W, H);
        for (int i = 0; i < W*H; i++) {
            fputc((fb[i]>>16)&255, o); fputc((fb[i]>>8)&255, o); fputc(fb[i]&255, o);
        }
        fclose(o);
    }
    return 0;
}
