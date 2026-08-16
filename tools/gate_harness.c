/* gate_harness.c — headless driver for the FULL engine (compositor + asm +
 * every pattern + audio module, no SDL window).  Used by the release gate.
 *
 *   gate smoke   W H N                 jd_frame N times (JD_MODE from env)
 *   gate battery W H START TOTAL EVERY per-sample: frame, mean luma, delta
 *                                      (mean abs channel delta vs previous
 *                                      frame), then fps for the whole run
 *   gate render  W H START END prefix f1 [f2 ...]
 *                                      run START..END, dump PPM at each fN
 *   gate run     W H START TOTAL       just run (JD_DEBUG=1 for the trace)
 *   gate audio   W H N                 jd_audio_init + tick + frame N times,
 *                                      print g_audio every 30 frames, dump
 *                                      last frame to /tmp/gate_audio.ppm
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/engine/jellydazzle.h"

extern void jd_frame(uint32_t *fb, int w, int h, int frame);

static double now_s(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
static double luma_of(const uint32_t *b, int n) {
    double s = 0;
    for (int i = 0; i < n; i++) {
        uint32_t c = b[i];
        s += ((c >> 16 & 255) * 54 + (c >> 8 & 255) * 183 + (c & 255) * 19) / 256.0;
    }
    return s / n;
}
static double contrast_of(const uint32_t *b, int n) {   /* luma stddev */
    double s = 0, s2 = 0;
    for (int i = 0; i < n; i++) {
        uint32_t c = b[i];
        double l = ((c >> 16 & 255) * 54 + (c >> 8 & 255) * 183 + (c & 255) * 19) / 256.0;
        s += l; s2 += l * l;
    }
    double m = s / n; double v = s2 / n - m * m; return v > 0 ? __builtin_sqrt(v) : 0;
}
static double delta_of(const uint32_t *a, const uint32_t *b, int n) {
    uint64_t s = 0;
    for (int i = 0; i < n; i++) {
        uint32_t p = a[i], q = b[i];
        s += abs((int)(p >> 16 & 255) - (int)(q >> 16 & 255))
           + abs((int)(p >> 8 & 255) - (int)(q >> 8 & 255))
           + abs((int)(p & 255) - (int)(q & 255));
    }
    return (double)s / ((double)n * 3);
}
static void ppm(const char *path, const uint32_t *b, int w, int h) {
    FILE *o = fopen(path, "wb");
    if (!o) { perror(path); return; }
    fprintf(o, "P6\n%d %d\n255\n", w, h);
    unsigned char *row = malloc((size_t)w * 3);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint32_t c = b[(size_t)y * w + x];
            row[x*3] = c >> 16; row[x*3+1] = c >> 8; row[x*3+2] = c;
        }
        fwrite(row, 3, (size_t)w, o);
    }
    free(row); fclose(o);
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: see header\n"); return 2; }
    const char *cmd = argv[1];
    int w = atoi(argv[2]), h = atoi(argv[3]);
    int n = w * h;
    uint32_t *fb = calloc((size_t)n, 4), *prev = calloc((size_t)n, 4);

    if (!strcmp(cmd, "smoke")) {
        int N = atoi(argv[4]);
        int start = 0;
        for (int f = 0; f < N; f++) jd_frame(fb, w, h, start + f);
        printf("ok luma=%.1f\n", luma_of(fb, n));
        return 0;
    }
    if (!strcmp(cmd, "run")) {
        int start = atoi(argv[4]), total = atoi(argv[5]);
        for (int f = 0; f < total; f++) jd_frame(fb, w, h, start + f);
        return 0;
    }
    if (!strcmp(cmd, "maxdelta")) {
        /* every-frame strobe scan of the live engine (or JD_MODE): max
         * single-step delta, where, how many steps >= 8, and mean */
        int start = atoi(argv[4]), total = atoi(argv[5]);
        double dmax = 0, dsum = 0; int fmax = 0, ge8 = 0, ge4 = 0;
        jd_frame(fb, w, h, start);
        for (int f = 1; f < total; f++) {
            memcpy(prev, fb, (size_t)n * 4);
            jd_frame(fb, w, h, start + f);
            double d = delta_of(prev, fb, n);
            if (d > dmax) { dmax = d; fmax = start + f; }
            if (d >= 8) ge8++;
            if (d >= 4) ge4++;
            dsum += d;
        }
        printf("MAXDELTA %.2f@%d ge8=%d ge4=%d mean=%.2f\n", dmax, fmax, ge8, ge4, dsum / (total - 1));
        return 0;
    }
    if (!strcmp(cmd, "battery")) {
        int start = atoi(argv[4]), total = atoi(argv[5]), every = atoi(argv[6]);
        double t0 = now_s();
        for (int f = 0; f < total; f++) {
            int fr = start + f;
            if (f % every == every - 1 && f > 0) {
                memcpy(prev, fb, (size_t)n * 4);
                jd_frame(fb, w, h, fr);
                printf("S %d %.1f %.2f %.1f\n", fr, luma_of(fb, n), delta_of(prev, fb, n), contrast_of(fb, n));
            } else jd_frame(fb, w, h, fr);
        }
        double dt = now_s() - t0;
        printf("FPS %.1f\n", total / dt);
        return 0;
    }
    if (!strcmp(cmd, "render")) {
        int start = atoi(argv[4]), end = atoi(argv[5]);
        const char *prefix = argv[6];
        int nd = argc - 7;
        for (int f = start; f <= end; f++) {
            jd_frame(fb, w, h, f);
            for (int k = 0; k < nd; k++) if (atoi(argv[7 + k]) == f) {
                char p[512]; snprintf(p, sizeof p, "%s_%d.ppm", prefix, f);
                ppm(p, fb, w, h);
                printf("F %d luma=%.1f\n", f, luma_of(fb, n));
            }
        }
        return 0;
    }
    if (!strcmp(cmd, "audio")) {
        int N = atoi(argv[4]);
        int on = jd_audio_init();
        printf("audio_init=%d\n", on);
        for (int f = 0; f < N; f++) {
            jd_audio_tick();
            jd_frame(fb, w, h, 1000 + f);
            if (f % 30 == 0)
                printf("f=%d live=%u level=%u bass=%u mid=%u treble=%u beat=%u bpm=%u\n",
                       f, g_audio.live, g_audio.level, g_audio.bass, g_audio.mid,
                       g_audio.treble, g_audio.beat, g_audio.bpm_q8);
            struct timespec ts = { 0, 16000000 }; nanosleep(&ts, NULL);
        }
        ppm("/tmp/gate_audio.ppm", fb, w, h);
        jd_audio_close();
        return 0;
    }
    fprintf(stderr, "unknown cmd\n"); return 2;
}
