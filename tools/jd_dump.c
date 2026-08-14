/* jd_dump.c — whole-engine harness for bridge.c (no SDL).
 *
 *   clang -O2 -I. tools/jd_dump.c bridge.c patterns_c/pattern_*.c \
 *         patterns_c/registry.c draw.s -o /tmp/jd_dump -lm
 *
 *   /tmp/jd_dump run START COUNT [ppm@N,N,...]
 *       renders COUNT frames from START, prints mean/peak per-channel
 *       frame-to-frame delta, ms/frame percentiles and implied fps, and
 *       writes /tmp/jd_NNNNNN.ppm at each requested frame.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define W 1280
#define H 960

extern void jd_frame(uint32_t *fb, int w, int h, int frame);

static uint32_t fb[W * H], prev[W * H];

static double now_ms(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static void ppm(const uint32_t *b, int f) {
    char path[64]; snprintf(path, sizeof path, "/tmp/jd_%06d.ppm", f);
    FILE *o = fopen(path, "wb");
    fprintf(o, "P6\n%d %d\n255\n", W, H);
    for (int i = 0; i < W * H; i++) {
        fputc((b[i] >> 16) & 255, o); fputc((b[i] >> 8) & 255, o); fputc(b[i] & 255, o);
    }
    fclose(o); fprintf(stderr, "wrote %s\n", path);
}

static double meanluma(const uint32_t *b, double *sigma) {
    double s = 0, s2 = 0;
    for (int i = 0; i < W * H; i += 7) {
        uint32_t c = b[i];
        double l = (((c >> 16) & 255) * 77 + ((c >> 8) & 255) * 150 + (c & 255) * 29) / 256.0;
        s += l; s2 += l * l;
    }
    int n = (W * H + 6) / 7;
    double m = s / n;
    if (sigma) *sigma = (s2 / n - m * m) > 0 ? __builtin_sqrt(s2 / n - m * m) : 0;
    return m;
}

static int cmpd(const void *a, const void *b) {
    double x = *(const double*)a, y = *(const double*)b;
    return x < y ? -1 : x > y;
}

int main(int argc, char **argv) {
    int start = argc > 2 ? atoi(argv[2]) : 0;
    int count = argc > 3 ? atoi(argv[3]) : 300;
    int shots[64], ns = 0;
    if (argc > 4) {
        char *p = strtok(argv[4], ",");
        while (p && ns < 64) { shots[ns++] = atoi(p); p = strtok(NULL, ","); }
    }
    int skip = getenv("JD_SKIP") ? atoi(getenv("JD_SKIP")) : 0;
    double *ms = malloc(sizeof(double) * count);
    int mn = 0;
    double dsum = 0, dmax = 0; int dn = 0;
    double lsum = 0, ssum = 0;
    int over8 = 0, worstf = -1, caught = 0;

    for (int k = 0; k < count; k++) {
        int f = start + k;
        double t0 = now_ms();
        jd_frame(fb, W, H, f);
        double el = now_ms() - t0;
        if (k >= skip) ms[mn++] = el;
        if (k > skip) {
            uint64_t s = 0;
            for (int i = 0; i < W * H; i += 3) {
                uint32_t c = fb[i], p = prev[i];
                int dr = (int)((c >> 16) & 255) - (int)((p >> 16) & 255);
                int dg = (int)((c >>  8) & 255) - (int)((p >>  8) & 255);
                int db = (int)( c        & 255) - (int)( p        & 255);
                s += (dr < 0 ? -dr : dr) + (dg < 0 ? -dg : dg) + (db < 0 ? -db : db);
            }
            double d = (double)s / ((W * H / 3) * 3.0);
            dsum += d; dn++;
            if (d > dmax) { dmax = d; worstf = f; }
            if (d > 8.0) { over8++; fprintf(stderr, "OVER f=%d d=%.2f\n", f, d); }
            if (d > 20.0 && getenv("JD_CATCH") && !caught) { caught = 1; ppm(prev, f - 1); ppm(fb, f); }
        }
        memcpy(prev, fb, sizeof fb);
        double sg; lsum += meanluma(fb, &sg); ssum += sg;
        for (int i = 0; i < ns; i++) if (shots[i] == f) ppm(fb, f);
    }
    qsort(ms, mn, sizeof(double), cmpd);
    count = mn;
    printf("frames        %d  (%d..%d)\n", count, start, start + count - 1);
    printf("delta         mean %.3f  peak %.3f (frame %d)  frames>8: %d\n",
           dsum / (dn ? dn : 1), dmax, worstf, over8);
    printf("ms/frame      p50 %.2f  p90 %.2f  p99 %.2f  max %.2f\n",
           ms[count / 2], ms[count * 9 / 10], ms[count * 99 / 100], ms[count - 1]);
    printf("fps           p50 %.1f  worst %.1f\n", 1000.0 / ms[count / 2],
           1000.0 / ms[count - 1]);
    printf("image         mean luma %.1f  mean sigma %.1f\n", lsum / count, ssum / count);
    return over8 ? 1 : 0;
}
