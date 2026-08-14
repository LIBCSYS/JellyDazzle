/* ============================================================
 * lab/qa/variety.c — LAUNCH VARIETY RIG
 *
 * Simulates ONE launch of JellyDazzle: engine_init at a random start
 * frame (exactly what main.c does — `rand() & 0x3FFFFF`), then N frames
 * of real jd_frame() at the real resolution, recording every routine
 * that ever becomes live in any slot and every palette scheme the shared
 * ramp walks through.
 *
 * It #includes bridge.c rather than linking against it, so the rig can
 * read the engine's file-static state (g_L, g_st, g_bag, g_run, the
 * probe cursor, scheme_at) WITHOUT a single edit to bridge.c.  That
 * matters: bridge.c is under concurrent repair and must stay untouched.
 *
 * build:
 *   clang -O2 -I. -DJD_NS=$(expr $(stat -f%z palette.bin) / 131072) \
 *       lab/qa/variety.c patterns_c/pattern_*.c patterns_c/registry.c \
 *       draw.s -o /tmp/variety -lm
 * run:
 *   /tmp/variety <startframe> [frames] [w] [h]
 *
 * stdout is machine-readable, one record per line:
 *   RUN     <startframe> <g_run> <w> <h> <frames>
 *   SPAWN   <k> <slot> <routine> <role> <repeat?>
 *   SCHEME  <k> <scheme>
 *   PROBE   <k> <routines_probed>          (first frame the sweep ends)
 *   SUM     <startframe> <distinct_routines> <spawns> <repeats>
 *           <distinct_schemes> <probe_done_frame> <first_rt>
 *   RTS     <routine> <routine> ...        (sorted, distinct)
 *   PALS    <scheme> <scheme> ...          (sorted, distinct)
 * ============================================================ */

#include "bridge.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int start  = argc > 1 ? (int)strtoul(argv[1], NULL, 10) : 0;
    int frames = argc > 2 ? atoi(argv[2]) : 3600;
    int W      = argc > 3 ? atoi(argv[3]) : 1280;
    int H      = argc > 4 ? atoi(argv[4]) : 960;

    uint32_t *fb = (uint32_t *)calloc((size_t)W * H, 4);
    if (!fb) return 1;

    static unsigned char seen[JD_MAXR];
    static unsigned char pseen[256];
    /* A TENANCY is (routine, t_in), not (routine, slot).  When the ground
     * retires, sched_tick SWAPS g_L[0] with the shadow slot, so the very
     * same layer reappears under a different slot index — counting that as
     * a second spawn overstates both the spawn count and the repeat count
     * (it put a phantom repeat in 12 of 12 launches before this was fixed).
     * t_in is set once at spawn and travels with the layer through the
     * swap, so (routine, t_in) identifies the tenancy across the handover. */
    int pv_rt[JD_NBUF], pv_in[JD_NBUF];
    for (int i = 0; i < JD_NBUF; i++) { pv_rt[i] = -1; pv_in[i] = 0; }

    int nspawn = 0, nrepeat = 0, ndistinct = 0, npal = 0;
    int probe_frame = -1, first_rt = -1;

    /* frame 0 also runs engine_init, which is where g_run is drawn */
    for (int k = 0; k < frames; k++) {
        int f = start + k;
        jd_frame(fb, W, H, f);

        if (k == 0)
            printf("RUN %d %u %d %d %d\n", start, g_run, W, H, frames);

        if (probe_frame < 0 && g_probe_done) {
            probe_frame = k;
            printf("PROBE %d %d\n", k, g_probe_i);
        }

        for (int s = 0; s < JD_NBUF; s++) {
            int rt = g_L[s].live ? g_L[s].routine : -1;
            int ti = g_L[s].live ? g_L[s].t_in : 0;
            if (rt >= 0) {
                int carried = 0;                    /* seen last frame anywhere */
                for (int j = 0; j < JD_NBUF; j++)
                    if (pv_rt[j] == rt && pv_in[j] == ti) { carried = 1; break; }
                if (!carried) {
                    int rep = seen[rt] ? 1 : 0;
                    if (!rep) { seen[rt] = 1; ndistinct++; }
                    else nrepeat++;
                    if (first_rt < 0) first_rt = rt;
                    nspawn++;
                    printf("SPAWN %d %d %d %d %d\n", k, s, rt, g_st[rt].role, rep);
                }
            }
        }
        for (int s = 0; s < JD_NBUF; s++) {
            pv_rt[s] = g_L[s].live ? g_L[s].routine : -1;
            pv_in[s] = g_L[s].live ? g_L[s].t_in : 0;
        }

        int A = scheme_at((uint32_t)f >> 10);
        if (A >= 0 && A < 256 && !pseen[A]) {
            pseen[A] = 1; npal++;
            printf("SCHEME %d %d\n", k, A);
        }
    }

    /* how far the (wall-clock-budgeted) probe sweep actually got, and how
     * many routines were therefore ELIGIBLE to be scheduled at all */
    int nprobed = 0;
    for (int i = 0; i < g_nr; i++) if (g_st[i].probed) nprobed++;
    printf("PROG %d %d %d %u %u %u %u\n", g_probe_i, nprobed, g_probe_done,
           g_bag[0].n, g_bag[1].n, g_bag[2].n, g_bag[3].n);

    printf("SUM %d %d %d %d %d %d %d\n",
           start, ndistinct, nspawn, nrepeat, npal, probe_frame, first_rt);

    printf("ELIG");
    for (int i = 0; i < g_nr; i++) if (g_st[i].probed) printf(" %d", i);
    printf("\n");

    printf("RTS");
    for (int i = 0; i < JD_MAXR; i++) if (seen[i]) printf(" %d", i);
    printf("\nPALS");
    for (int i = 0; i < 256; i++) if (pseen[i]) printf(" %d", i);
    printf("\n");

    free(fb);
    return 0;
}
