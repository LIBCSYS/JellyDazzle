/* jd_dump.c — whole-engine measurement harness for the v2.1 release notes.
 * Includes bridge.c so it can read the probe table (g_st) directly.
 *
 *   ./dump stats            dump the probe table for all routines
 *   ./dump run N            run N frames; print fps + composited motion stats
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "bridge.c"

#define W 1280
#define H 960
static uint32_t fb[W*H], prev[W*H];

static const char *ROLE[] = { "GROUND", "FIELD", "FIGURE", "SPARK" };

int main(int argc, char **argv)
{
    const char *cmd = argc > 1 ? argv[1] : "stats";

    if (!strcmp(cmd, "stats")) {
        /* run frames until the incremental probe sweep finishes */
        for (int f = 0; f < 20000 && !g_probe_done; f++) jd_frame(fb, W, H, f);
        printf("routines %d schemes %d probe_done %d\n", g_nr, g_ns, g_probe_done);
        printf("rt,kind,role,cls,dark,luma,sat,delta,cost_ms,probed\n");
        for (int i = 0; i < g_nr; i++) {
            jd_stat *s = &g_st[i];
            printf("%d,%s,%s,%s,%u,%u,%u,%.3f,%.3f,%u\n",
                   i, i < JD_NASM ? "asm" : "pattern",
                   ROLE[s->role], s->cls == C_CANVAS ? "canvas" : "pure",
                   s->dark, s->luma, s->sat,
                   s->delta_q8 / 256.0, s->cost_q8 / 256.0, s->probed);
        }
        return 0;
    }

    if (!strcmp(cmd, "run")) {
        int n = argc > 2 ? atoi(argv[2]) : 3600;
        /* warm: finish the probe first so it does not pollute the fps number */
        int f = 0;
        for (; f < 20000 && !g_probe_done; f++) jd_frame(fb, W, H, f);
        int probe_frames = f;

        struct timespec a, b;
        double dsum = 0, dmax = 0; long dn = 0;
        long over8 = 0;
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < n; i++, f++) {
            memcpy(prev, fb, sizeof fb);
            jd_frame(fb, W, H, f);
            if (i == 0) continue;
            long s = 0;
            for (int p = 0; p < W*H; p++) {
                uint32_t u = fb[p], v = prev[p];
                s += labs((long)((u>>16)&255) - (long)((v>>16)&255))
                   + labs((long)((u>> 8)&255) - (long)((v>> 8)&255))
                   + labs((long)( u     &255) - (long)( v     &255));
            }
            double d = (double)s / ((double)W*H*3);
            dsum += d; dn++;
            if (d > dmax) dmax = d;
            if (d > 8.0) over8++;
        }
        clock_gettime(CLOCK_MONOTONIC, &b);
        double sec = (b.tv_sec-a.tv_sec) + (b.tv_nsec-a.tv_nsec)/1e9;
        /* the delta loop is not free; report render-only fps separately */
        printf("probe_frames %d\n", probe_frames);
        printf("frames %d wall_s %.2f (includes delta measurement)\n", n, sec);
        printf("motion_mean %.3f motion_max %.3f frames_over_8 %ld of %ld\n",
               dsum/dn, dmax, over8, dn);
        return 0;
    }

    if (!strcmp(cmd, "fps")) {
        int n = argc > 2 ? atoi(argv[2]) : 1800;
        int f = 0;
        for (; f < 20000 && !g_probe_done; f++) jd_frame(fb, W, H, f);
        struct timespec a, b;
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < n; i++, f++) jd_frame(fb, W, H, f);
        clock_gettime(CLOCK_MONOTONIC, &b);
        double sec = (b.tv_sec-a.tv_sec) + (b.tv_nsec-a.tv_nsec)/1e9;
        printf("render_only frames %d wall_s %.3f ms_per_frame %.3f fps %.1f\n",
               n, sec, sec*1000.0/n, n/sec);
        return 0;
    }

    if (!strcmp(cmd, "pal")) {
        /* palette bag: how evenly are the schemes used, and does a leg seam
         * ever land two perceptually-close schemes next to each other? */
        int legs = argc > 2 ? atoi(argv[2]) : 0;
        jd_frame(fb, W, H, 0);                     /* force init */
        if (!legs) legs = g_ns * 4;
        int *use = calloc(g_ns, sizeof *use);
        int prev = -1, adjacent_close = 0;
        double dsum = 0;
        for (int l = 0; l < legs; l++) {
            int s = scheme_at((uint32_t)l);
            use[s]++;
            if (prev >= 0) { double d = pal_dist(prev, s); dsum += d;
                             if (d < g_pthresh) adjacent_close++; }
            prev = s;
        }
        int mn = 1 << 30, mx = 0, unused = 0;
        for (int i = 0; i < g_ns; i++) {
            if (use[i] < mn) mn = use[i];
            if (use[i] > mx) mx = use[i];
            if (!use[i]) unused++;
        }
        printf("schemes %d legs %d (%d epochs)\n", g_ns, legs, legs / g_ns);
        printf("usage min %d max %d spread %d unused %d\n", mn, mx, mx - mn, unused);
        printf("leg seams: mean perceptual distance %.4f threshold %.4f "
               "seams below threshold %d of %d\n",
               dsum / (legs - 1), g_pthresh, adjacent_close, legs - 1);
        free(use);
        return 0;
    }

    fprintf(stderr, "usage: stats | run N | fps N | pal [legs]\n");
    return 1;
}
