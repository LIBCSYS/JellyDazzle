/* engine_dump.c — whole-engine smoke harness.
 *
 * Links the REAL engine (bridge.c + every patterns_c .c + draw.s) and drives
 * jd_frame() exactly the way main.c does, minus SDL.  One mode per
 * process so a crash in routine N is attributable and does not take the
 * rest of the sweep with it.
 *
 *   engine_dump <mode> <frames> [ppm_out]
 *
 * mode 0        = full v2.1 layer compositor (no JD_MODE override)
 * mode 1..200   = C plug-in pattern_NNN, solo
 * mode 1000+k   = asm routine k (0..23), solo
 *
 * mode_override() reads JD_MODE as: 1..jd_pattern_count selects a C
 * plug-in, anything else selects asm mode (v % 24).  So asm k is not
 * addressable as "k" — we send 240+k, which is >200 and 240%24==0.
 *
 * Framebuffer is bracketed by canary pages so a pattern that writes
 * outside w*h is caught here instead of corrupting someone's heap.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define W 1280
#define H 960
#define NPIX (W * H)
#define GUARD 4096                 /* u32 slots of canary on each side */
#define CANARY 0xDEADC0DEu

extern void jd_frame(uint32_t *fb, int width, int height, int frame);

int main(int argc, char **argv)
{
    int mode   = argc > 1 ? atoi(argv[1]) : 0;
    int frames = argc > 2 ? atoi(argv[2]) : 400;
    const char *ppm = argc > 3 ? argv[3] : NULL;

    /* mode_override() reads JD_MODE once, on the first jd_frame call */
    if (mode > 0) {
        char buf[32];
        snprintf(buf, sizeof buf, "%d",
                 mode >= 1000 ? 240 + ((mode - 1000) % 24) : mode);
        setenv("JD_MODE", buf, 1);
    } else {
        unsetenv("JD_MODE");
    }

    uint32_t *base = malloc((size_t)(NPIX + 2 * GUARD) * 4);
    if (!base) { fprintf(stderr, "OOM\n"); return 3; }
    for (int i = 0; i < NPIX + 2 * GUARD; i++) base[i] = CANARY;
    uint32_t *fb = base + GUARD;
    memset(fb, 0, (size_t)NPIX * 4);

    /* frame-to-frame motion signature, same sampling law as the engine's
     * own probe: 1024 sample points, mean absolute per-channel delta */
    static uint32_t prev[1024];
    int step = NPIX / 1024;
    double motion_sum = 0; int motion_n = 0;
    double peak = 0; int peak_f = -1, spikes = 0;
    int series = getenv("JD_SERIES") != NULL;
    uint64_t lum_sum = 0;
    int nonblack_frames = 0;

    for (int f = 0; f < frames; f++) {
        jd_frame(fb, W, H, 300 + f);

        uint64_t s = 0; uint64_t lum = 0; int nz = 0;
        for (int k = 0; k < 1024; k++) {
            uint32_t c = fb[k * step];
            int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
            lum += (uint64_t)(r + g + b);
            if (r | g | b) nz = 1;
            uint32_t p = prev[k];
            int dr = r - (int)((p >> 16) & 255);
            int dg = g - (int)((p >> 8) & 255);
            int db = b - (int)(p & 255);
            s += (uint32_t)abs(dr) + (uint32_t)abs(dg) + (uint32_t)abs(db);
            prev[k] = c;
        }
        if (nz) nonblack_frames++;
        lum_sum += lum;
        if (f > 0) {                       /* frame 0 delta is meaningless */
            double d = (double)s / (1024.0 * 3.0);
            motion_sum += d; motion_n++;
            if (d > peak) { peak = d; peak_f = f; }
            if (d > 8.0) spikes++;
            if (series) printf("  f=%d d=%.3f\n", f, d);
        }
    }

    /* canaries */
    int lo_hit = 0, hi_hit = 0;
    for (int i = 0; i < GUARD; i++) {
        if (base[i] != CANARY) lo_hit++;
        if (base[GUARD + NPIX + i] != CANARY) hi_hit++;
    }

    double mean_motion = motion_n ? motion_sum / motion_n : 0.0;
    double mean_lum    = (double)lum_sum / ((double)frames * 1024.0 * 3.0);

    if (ppm) {
        FILE *fp = fopen(ppm, "wb");
        if (fp) {
            fprintf(fp, "P6\n%d %d\n255\n", W, H);
            for (int i = 0; i < NPIX; i++) {
                uint32_t c = fb[i];
                fputc((c >> 16) & 255, fp);
                fputc((c >> 8) & 255, fp);
                fputc(c & 255, fp);
            }
            fclose(fp);
        }
    }

    printf("mode=%d frames=%d motion=%.3f peak=%.3f peak_f=%d spikes=%d "
           "lum=%.2f nonblack=%d guard_lo=%d guard_hi=%d\n",
           mode, frames, mean_motion, peak, peak_f, spikes, mean_lum,
           nonblack_frames, lo_hit, hi_hit);

    free(base);
    return (lo_hit || hi_hit) ? 4 : 0;
}
