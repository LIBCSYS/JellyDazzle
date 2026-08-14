/* jd_scan.c — motion audit of every routine in the library.
 *
 *   clang -O2 -I. tools/jd_scan.c bridge_stub.c patterns_c/pattern_*.c \
 *         patterns_c/registry.c draw.s -o /tmp/jd_scan -lm
 *
 *   /tmp/jd_scan [frames] [w] [h]
 *
 * Runs each routine on its own from sl==0 for `frames` frames and reports the
 * mean and the WORST single frame-to-frame per-channel delta.  The worst step
 * is the point: an accumulator that re-seeds its composition every few hundred
 * frames reads perfectly calm on a 48-frame probe and is a hard cut on screen.
 * Prints one CSV row per routine: rt,mean,max,argmax,period
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "jellydazzle.h"

extern const jd_pattern_fn jd_patterns[];
extern const int jd_pattern_count;
extern const uint32_t jd_palette[];
extern void draw_frame(uint32_t*, int, int, int);
extern uint32_t g_mode;

#define JD_NASM 24

static uint32_t *fb, *prev;

static double delta_of(const uint32_t *a, const uint32_t *b, int n)
{
    uint64_t s = 0;
    for (int i = 0; i < n; i++) {
        uint32_t c = a[i], p = b[i];
        int dr = (int)((c >> 16) & 255) - (int)((p >> 16) & 255);
        int dg = (int)((c >>  8) & 255) - (int)((p >>  8) & 255);
        int db = (int)( c        & 255) - (int)( p        & 255);
        s += (dr < 0 ? -dr : dr) + (dg < 0 ? -dg : dg) + (db < 0 ? -db : db);
    }
    return (double)s / (n * 3.0);
}

int main(int argc, char **argv)
{
    int nf = argc > 1 ? atoi(argv[1]) : 1500;
    int w  = argc > 2 ? atoi(argv[2]) : 640;
    int h  = argc > 3 ? atoi(argv[3]) : 480;
    int npix = w * h;
    fb   = malloc((size_t)npix * 4);
    prev = malloc((size_t)npix * 4);
    int nr = JD_NASM + jd_pattern_count;

    printf("rt,kind,idx,mean,max,argmax,over8\n");
    for (int rt = 0; rt < nr; rt++) {
        memset(fb, 0, (size_t)npix * 4);
        memset(prev, 0, (size_t)npix * 4);
        uint32_t seed = 0xC0FFEE11u + (uint32_t)rt;
        double sum = 0, mx = 0; int n = 0, arg = -1, over = 0;
        for (int f = 0; f < nf; f++) {
            if (rt < JD_NASM) { g_mode = (uint32_t)rt; draw_frame(fb, w, h, 300 + f); }
            else jd_patterns[rt - JD_NASM](fb, w, h, 300 + f, f, seed, jd_palette);
            if (f > 3) {
                double d = delta_of(fb, prev, npix);
                sum += d; n++;
                if (d > mx) { mx = d; arg = f; }
                if (d > 8.0) over++;
            }
            memcpy(prev, fb, (size_t)npix * 4);
        }
        printf("%d,%s,%d,%.4f,%.4f,%d,%d\n", rt,
               rt < JD_NASM ? "asm" : "pat",
               rt < JD_NASM ? rt : rt - JD_NASM + 1,
               n ? sum / n : 0.0, mx, arg, over);
        fflush(stdout);
    }
    return 0;
}
