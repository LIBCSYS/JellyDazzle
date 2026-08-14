/* Port-verification harness. Compile alongside ONE pattern:
 *   clang -O2 -DPATTERN=pattern_NNN harness.c pattern_NNN.c -o t
 * Usage:
 *   ./t render out.ppm START COUNT   render frames START..START+COUNT-1
 *                                    sequentially (sl = frame&2047), dump last
 *   ./t bench                        120 frames at 1280x960, prints fps
 *   ./t delta FRAME                  mean abs channel delta FRAME vs FRAME+1
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../jellydazzle.h"
#define W 1280
#define H 960
void PATTERN(uint32_t*, int, int, int, int, uint32_t, const uint32_t*);
static uint32_t fb[W*H], fb2[W*H], pal[32768];
static void load_pal(void) {
    FILE *f = fopen("../palette.bin", "rb");
    if (!f) f = fopen("palette.bin", "rb");
    if (!f) { fprintf(stderr, "no palette.bin\n"); exit(1); }
    fread(pal, 4, 32768, f); fclose(f);       /* scheme 0 as the test palette */
}
static void run(uint32_t *b, int frame) {
    PATTERN(b, W, H, frame, frame & 2047, 0xC0FFEE11u ^ (uint32_t)(frame >> 11), pal);
}
int main(int argc, char **argv) {
    load_pal();
    if (argc >= 2 && !strcmp(argv[1], "bench")) {
        struct timespec a, b; clock_gettime(CLOCK_MONOTONIC, &a);
        for (int f = 0; f < 120; f++) run(fb, f);
        clock_gettime(CLOCK_MONOTONIC, &b);
        double s = (b.tv_sec-a.tv_sec) + (b.tv_nsec-a.tv_nsec)/1e9;
        printf("%.1f fps\n", 120.0/s); return 0;
    }
    if (argc >= 3 && !strcmp(argv[1], "delta")) {
        int f0 = atoi(argv[2]);
        for (int f = f0 & ~2047; f <= f0; f++) run(fb, f);
        memcpy(fb2, fb, sizeof fb); run(fb2, f0 + 1);
        long s = 0;
        for (int i = 0; i < W*H; i++) {
            uint32_t p = fb[i], q = fb2[i];
            s += labs((long)((p>>16)&255)-(long)((q>>16)&255))
               + labs((long)((p>>8)&255)-(long)((q>>8)&255))
               + labs((long)(p&255)-(long)(q&255));
        }
        printf("mean delta %.2f\n", (double)s/(W*H*3)); return 0;
    }
    if (argc >= 5 && !strcmp(argv[1], "render")) {
        int start = atoi(argv[3]), count = atoi(argv[4]);
        for (int f = start; f < start + count; f++) run(fb, f);
        FILE *o = fopen(argv[2], "wb");
        fprintf(o, "P6\n%d %d\n255\n", W, H);
        for (int i = 0; i < W*H; i++) {
            fputc((fb[i]>>16)&255, o); fputc((fb[i]>>8)&255, o); fputc(fb[i]&255, o);
        }
        fclose(o); printf("ok\n"); return 0;
    }
    fprintf(stderr, "usage: render|bench|delta\n"); return 1;
}
