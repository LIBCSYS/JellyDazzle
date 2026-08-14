/* jd_sched_test.c — drives the real scheduler in bridge.c with stub
 * routines, so bag/palette statistics can be measured over hours of
 * simulated playback in seconds.
 *
 *   clang -O2 -I. tools/jd_sched_test.c bridge.c -o /tmp/jd_sched -lm
 *   JD_DEBUG=1 /tmp/jd_sched 400000 2>&1 >/dev/null | python3 ...
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../jellydazzle.h"

#define NP 100
#define W 128
#define H 96

/* a palette that is not flat, so the feature/threshold code is exercised */
/* bridge.c declares this const; defined non-const here so the test can fill
 * it.  Separate translation units, so the linker is content. */
uint32_t jd_palette[30 * 32768];

/* stub routines with a spread of coverage so all four role bags fill */
static void stub(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal, int k)
{
    (void)frame; (void)sl; (void)seed;
    int cov = (k % 4 == 0) ? h : (k % 4 == 1) ? h * 3 / 5
            : (k % 4 == 2) ? h / 4 : h / 12;
    uint32_t c = pal[(k * 977) & 0x7FFF] | 0xFF000000u;
    for (int y = 0; y < h; y++) {
        uint32_t v = y < cov ? c : 0xFF000000u;
        for (int x = 0; x < w; x++) fb[y * w + x] = v ^ ((uint32_t)(x + frame) & 3);
    }
}
#define DEF(n) static void q##n(uint32_t *f,int w,int h,int fr,int sl,uint32_t s,const uint32_t *p){stub(f,w,h,fr,sl,s,p,0x##n);}
#define D10(b) DEF(b##0) DEF(b##1) DEF(b##2) DEF(b##3) DEF(b##4) DEF(b##5) DEF(b##6) DEF(b##7) DEF(b##8) DEF(b##9)
D10(0) D10(1) D10(2) D10(3) D10(4) D10(5) D10(6) D10(7) D10(8) D10(9)
#define R10(b) q##b##0,q##b##1,q##b##2,q##b##3,q##b##4,q##b##5,q##b##6,q##b##7,q##b##8,q##b##9
const jd_pattern_fn jd_patterns[NP] = { R10(0), R10(1), R10(2), R10(3), R10(4),
                                        R10(5), R10(6), R10(7), R10(8), R10(9) };
const int jd_pattern_count = NP;

extern uint32_t g_mode;
void draw_frame(uint32_t *fb, int w, int h, int frame)
{
    stub(fb, w, h, frame, 0, 0, jd_palette, (int)g_mode * 7 + 1);
}

extern void jd_frame(uint32_t*, int, int, int);
static uint32_t fb[W * H];

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 200000;
    for (int s = 0; s < 30; s++)
        for (int i = 0; i < 32768; i++) {
            int r = (i * (s + 3)) & 255, g = (i * (s + 7) / 3) & 255, b = (i * (s + 11) / 5) & 255;
            jd_palette[s * 32768 + i] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    static uint32_t prev[W * H];
    int leg = 0, seg = 0, other = 0; double worst = 0; int worstf = 0;
    for (int f = 0; f < n; f++) {
        jd_frame(fb, W, H, 500000 + f);
        if (f) {
            long s2 = 0;
            for (int i = 0; i < W * H; i++) {
                uint32_t c = fb[i], p = prev[i];
                s2 += labs((long)((c>>16)&255)-(long)((p>>16)&255))
                    + labs((long)((c>>8)&255)-(long)((p>>8)&255))
                    + labs((long)(c&255)-(long)(p&255));
            }
            double d = (double)s2 / (W * H * 3);
            if (d > worst) { worst = d; worstf = 500000 + f; }
            if (d > 8.0) {
                int fr = 500000 + f;
                if ((fr & 1023) == 0) leg++;
                else if ((fr & 2047) == 0) seg++;
                else { other++; if (other < 12) fprintf(stderr, "BREAK f=%d d=%.1f leg%%=%d\n", fr, d, fr & 1023); }
            }
        }
        memcpy(prev, fb, sizeof fb);
    }
    printf("worst delta %.2f at %d ; over8: leg-boundary %d  seg-boundary %d  other %d\n",
           worst, worstf, leg, seg, other);
    return 0;
}
