/* jd_qc.c — performance & motion QC harness for the v2.1 engine.
 *
 *   clang -O2 -I. tools/jd_qc.c bridge.c patterns_c/pattern_*.c \
 *         patterns_c/registry.c draw.s -o /tmp/jd_qc -lm
 *
 *   /tmp/jd_qc START COUNT [csv_path]
 *
 * Renders COUNT frames at 1280x960 from START and writes one CSV row per
 * frame: frame,ms,delta  (delta = mean per-channel |fb - prev| over a
 * 1/3-pixel stride, exactly the metric the house rule names).  Summary goes
 * to stdout; peak RSS is reported from getrusage.
 *
 * Frame 0 of a run has no predecessor and is excluded from the delta stats.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/resource.h>

#define W 1280
#define H 960

extern void jd_frame(uint32_t *fb, int w, int h, int frame);

static uint32_t fb[W * H], prev[W * H];

static double now_ms(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static int cmpd(const void *a, const void *b) {
    double x = *(const double*)a, y = *(const double*)b;
    return x < y ? -1 : x > y;
}

int main(int argc, char **argv) {
    int start = argc > 1 ? atoi(argv[1]) : 0;
    int count = argc > 2 ? atoi(argv[2]) : 600;
    const char *csv = argc > 3 ? argv[3] : NULL;
    FILE *out = csv ? fopen(csv, "w") : NULL;
    if (out) fprintf(out, "frame,ms,delta\n");

    double *ms = malloc(sizeof(double) * count);
    double *dl = malloc(sizeof(double) * count);
    int mn = 0, dn = 0;
    double dmax = 0; int worstf = -1, over8 = 0;

    for (int k = 0; k < count; k++) {
        int f = start + k;
        double t0 = now_ms();
        jd_frame(fb, W, H, f);
        double el = now_ms() - t0;
        ms[mn++] = el;
        double d = -1.0;
        if (k > 0) {
            uint64_t s = 0;
            for (int i = 0; i < W * H; i += 3) {
                uint32_t c = fb[i], p = prev[i];
                int dr = (int)((c >> 16) & 255) - (int)((p >> 16) & 255);
                int dg = (int)((c >>  8) & 255) - (int)((p >>  8) & 255);
                int db = (int)( c        & 255) - (int)( p        & 255);
                s += (dr < 0 ? -dr : dr) + (dg < 0 ? -dg : dg) + (db < 0 ? -db : db);
            }
            d = (double)s / ((W * H / 3) * 3.0);
            dl[dn++] = d;
            if (d > dmax) { dmax = d; worstf = f; }
            if (d > 8.0) over8++;
        }
        memcpy(prev, fb, sizeof fb);
        if (out) fprintf(out, "%d,%.4f,%.4f\n", f, el, d);
    }
    if (out) fclose(out);

    double dsum = 0; for (int i = 0; i < dn; i++) dsum += dl[i];
    qsort(ms, mn, sizeof(double), cmpd);
    qsort(dl, dn, sizeof(double), cmpd);
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);

    printf("range         %d..%d  (%d frames)\n", start, start + count - 1, count);
    printf("ms/frame      p50 %.2f  p90 %.2f  p99 %.2f  max %.2f\n",
           ms[mn / 2], ms[mn * 9 / 10], ms[mn * 99 / 100], ms[mn - 1]);
    printf("fps           p50 %.1f  p90 %.1f  worst %.1f\n",
           1000.0 / ms[mn / 2], 1000.0 / ms[mn * 9 / 10], 1000.0 / ms[mn - 1]);
    printf("delta         mean %.3f  median %.3f  p90 %.3f  peak %.3f (f=%d)  over8 %d\n",
           dsum / (dn ? dn : 1), dl[dn / 2], dl[dn * 9 / 10], dmax, worstf, over8);
    printf("maxrss        %.1f MB\n", ru.ru_maxrss / (1024.0 * 1024.0));
    return over8 ? 1 : 0;
}
