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
 *   gate keys    W H START TOTAL      3.0: press C at 1/3 and S at 2/3 of the
 *                                      run and report what each one changed,
 *                                      plus the worst single-frame delta in
 *                                      the second after the press (a key may
 *                                      change the picture; it may not strobe)
 *   gate contrast W H START TOTAL [EVERY] [frames.csv] [tenancies.csv]
 *                                    accent-vs-ground visibility, ground
 *                                    luma distribution and a fog metric
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

/* ============================================================================
 * contrast (3.0.4) — is the accent actually VISIBLE, and how much of the
 * problem is the ground?
 * ============================================================================
 *
 *   gate contrast W H START TOTAL [EVERY] [frames.csv] [tenancies.csv]
 *
 * The complaint this answers is "foggy clutter where the accent is barely
 * visible".  Three separate claims hide in that sentence and the harness
 * keeps them apart:
 *
 *   1. the accent is there but SWAMPED   -> accent visibility index V
 *   2. the ground is too BRIGHT to sit under an accent at all
 *                                        -> ground luma distribution, headroom
 *   3. the whole frame is FOGGY          -> low local contrast over a bright
 *                                           base, measured per tile
 *
 * HOW THE ACCENT IS ISOLATED.  Not by running the engine twice.  The
 * compositor adapts to measured frame cost (g_ewma_ms, g_mood, g_hot), so two
 * runs from the same start frame diverge within seconds and any cross-run
 * diff measures the divergence.  Instead jd_tap fires INSIDE the frame: once
 * when the ground composite is down and once after each overlay is blended.
 * The composite of a given frame's layer buffers is a pure function, so
 * "what was underneath the accent on this exact frame" is exact, not a
 * reconstruction.  Every number below is a within-frame comparison; run-to-run
 * divergence can only change WHICH frames get sampled, which makes it a
 * sampling problem with ordinary statistics, not a measurement problem.
 *
 * WHAT V MEANS.  For every pixel the accent actually changed:
 *
 *      S   = signal   = sqrt(dL^2 + (0.5*d_rg)^2 + (0.4*d_yb)^2)
 *      thr = threshold = 0.5*sigma_local + 0.05*pedestal_local + 2
 *      V   = S / thr
 *
 * dL is the Rec.709 luma step the accent makes (same weights as luma_of
 * above); d_rg and d_yb are the red-green and yellow-blue opponent steps, so
 * an accent that reads by HUE at equal luma is not scored as invisible.  They
 * are weighted down because chromatic contrast sensitivity falls away much
 * faster with spatial frequency than luminance does; 0.5/0.4 is a judgement
 * call and is the single most calibratable constant here.
 *
 * The threshold is a plain masking model: 0.5*sigma is contrast masking by
 * the texture immediately under the accent (Legge-Foley slope, rounded),
 * 0.05*pedestal is the Weber term — 5%, deliberately several times the ~1%
 * textbook JND, because a soft-edged moving overlay glimpsed on animated
 * content is nothing like a static edge under fixation — and +2 code values
 * is the 8-bit floor so a black underlay cannot make V infinite.
 *
 *      V >= 1  the accent equals its local detection threshold: just reads
 *      V >= 2  clearly reads
 *      V <  1  swamped — present in the buffer, absent to the eye
 *
 * These are gamma-encoded code values, not linear luminance.  That is on
 * purpose: the judgement a viewer makes is a LIGHTNESS judgement and lightness
 * tracks the encoded value far better than the linear one, and it keeps every
 * number in this report directly comparable with a pixel value.
 *
 * WHAT IT CANNOT MEASURE.  Whether the accent is BEAUTIFUL, whether it reads
 * as an accent rather than as noise, and whether the eye is drawn to it.  V
 * says only that a difference is above threshold.  Calibrate it once against
 * J's eyes with `render` on the frames this command ranks best/median/worst,
 * then trust the ordering.
 */
#define CT_NSLOT   5              /* JD_NBUF: slots 0..3 plus the shadow ground */
#define CT_ALL     CT_NSLOT       /* pseudo-series: all overlays taken together */
#define CT_NSER    (CT_NSLOT + 1)
#define CT_ACCENT  2              /* JD_NSLOT names them base, mid, accent, spark */
#define CT_VBINS   161            /* V in [0,8) at 0.05 wide; 160 = overflow     */
#define CT_VSCALE  20.0           /* bins per unit of V                          */
#define CT_COVMIN  0.02           /* below 2% of the frame it is not an accent   */
#define CT_FOGC    0.15           /* tile RMS contrast under this = flat         */
#define CT_FOGL    90.0           /* ... and over this luma = flat AND bright    */

static double ct_luma(uint32_t c) {      /* same weights as luma_of() above */
    return ((c >> 16 & 255) * 54 + (c >> 8 & 255) * 183 + (c & 255) * 19) / 256.0;
}

/* ---- tile grid ------------------------------------------------------------
 * Masking is LOCAL.  An accent is swamped by the texture directly under it,
 * never by the average of the whole screen, so sigma and the pedestal are
 * taken from the tile the pixel falls in.  A tile of ~1/24 of the shorter
 * side is roughly 1.5 degrees of visual angle on a full-screen display, which
 * is the scale these soft, large-blob overlays actually work at. */
static int     ct_tsz, ct_ntx, ct_nty, ct_nt;
static float  *ct_tm, *ct_tsd;                    /* per-tile mean / stddev */
static double *ct_s1, *ct_s2; static long *ct_cn;

static void ct_tiles(const uint32_t *b, int w, int h)
{
    for (int t = 0; t < ct_nt; t++) { ct_s1[t] = 0; ct_s2[t] = 0; ct_cn[t] = 0; }
    for (int y = 0; y < h; y++) {
        int ty = y / ct_tsz; if (ty >= ct_nty) ty = ct_nty - 1;
        const uint32_t *row = b + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            int tx = x / ct_tsz; if (tx >= ct_ntx) tx = ct_ntx - 1;
            int t = ty * ct_ntx + tx;
            double l = ct_luma(row[x]);
            ct_s1[t] += l; ct_s2[t] += l * l; ct_cn[t]++;
        }
    }
    for (int t = 0; t < ct_nt; t++) {
        double m = ct_cn[t] ? ct_s1[t] / ct_cn[t] : 0.0;
        double v = ct_cn[t] ? ct_s2[t] / ct_cn[t] - m * m : 0.0;
        ct_tm[t] = (float)m; ct_tsd[t] = (float)(v > 0 ? __builtin_sqrt(v) : 0);
    }
}

/* percentile out of a fixed-width histogram */
static double ct_pct(const long *hist, int nb, long tot, double p, double scale)
{
    if (tot <= 0) return 0.0;
    long want = (long)(p * (double)tot), acc = 0;
    for (int i = 0; i < nb; i++) { acc += hist[i]; if (acc > want) return (i + 0.5) / scale; }
    return (nb - 0.5) / scale;
}

typedef struct {
    double cov;          /* fraction of the frame this layer actually changed */
    double v50, v90;     /* visibility index over that footprint              */
    double vf1, vf2;     /* fraction of the footprint at V>=1 and V>=2        */
    double sgn, sig, ped, head;   /* mean signal, masker, pedestal, headroom  */
} ct_meas;

/* Compare the composite before and after one layer went on.  Everything is
 * measured only where the layer CHANGED something: averaging a strong accent
 * over the 96% of the frame it never touched is how an accent gets scored as
 * invisible when it is merely small. */
static void ct_measure(const uint32_t *under, const uint32_t *over,
                       int w, int h, ct_meas *m)
{
    long hist[CT_VBINS]; memset(hist, 0, sizeof hist);
    long touched = 0, npix = (long)w * h;
    double s_sgn = 0, s_sig = 0, s_ped = 0, s_head = 0;
    memset(m, 0, sizeof *m);
    ct_tiles(under, w, h);
    for (int y = 0; y < h; y++) {
        int ty = y / ct_tsz; if (ty >= ct_nty) ty = ct_nty - 1;
        for (int x = 0; x < w; x++) {
            size_t i = (size_t)y * w + x;
            uint32_t a = under[i], b = over[i];
            if (a == b) continue;
            int ar = a >> 16 & 255, ag = a >> 8 & 255, ab = a & 255;
            int br = b >> 16 & 255, bg = b >> 8 & 255, bb = b & 255;
            double dL  = ((br - ar) * 54 + (bg - ag) * 183 + (bb - ab) * 19) / 256.0;
            double drg = (double)((br - bg) - (ar - ag));
            double dyb = ((br + bg) * 0.5 - bb) - ((ar + ag) * 0.5 - ab);
            double S   = __builtin_sqrt(dL * dL + 0.25 * drg * drg + 0.16 * dyb * dyb);
            if (S < 0.5) continue;         /* under half a code value: not a change */
            int tx = x / ct_tsz; if (tx >= ct_ntx) tx = ct_ntx - 1;
            int t  = ty * ct_ntx + tx;
            double sd = ct_tsd[t], pe = ct_tm[t];
            double V  = S / (0.5 * sd + 0.05 * pe + 2.0);
            int bin = (int)(V * CT_VSCALE); if (bin >= CT_VBINS) bin = CT_VBINS - 1;
            hist[bin]++; touched++;
            s_sgn += S; s_sig += sd; s_ped += pe; s_head += (255.0 - pe) / 255.0;
        }
    }
    if (!touched) return;
    m->cov  = (double)touched / (double)npix;
    m->v50  = ct_pct(hist, CT_VBINS, touched, 0.50, CT_VSCALE);
    m->v90  = ct_pct(hist, CT_VBINS, touched, 0.90, CT_VSCALE);
    long ge1 = 0, ge2 = 0;
    for (int i = (int)(1.0 * CT_VSCALE); i < CT_VBINS; i++) ge1 += hist[i];
    for (int i = (int)(2.0 * CT_VSCALE); i < CT_VBINS; i++) ge2 += hist[i];
    m->vf1 = (double)ge1 / (double)touched;
    m->vf2 = (double)ge2 / (double)touched;
    m->sgn = s_sgn / touched; m->sig = s_sig / touched;
    m->ped = s_ped / touched; m->head = s_head / touched;
}

/* ---- looking at what was scored ------------------------------------------
 * You cannot go back and re-render an interesting frame.  The engine adapts
 * to measured frame cost, so `gate render` on the same start frame produces a
 * DIFFERENT show — this is the trap that ended one earlier investigation.  So
 * the extremes are written out DURING the run that scored them.
 *
 * JD_CT_DUMP=/tmp/ct   writes six files, overwritten each time a new extreme
 * appears, so what survives to the end of the run is the genuine best and
 * worst the run produced:
 *      _best.ppm  _best_under.ppm  _best_vmap.ppm
 *      _worst.ppm _worst_under.ppm _worst_vmap.ppm
 * plus _fog.ppm, the foggiest delivered frame.
 *
 * The vmap is the calibration instrument: untouched pixels are the underlay
 * at half brightness, and every pixel the accent changed is tinted by its V —
 * red below 1 (swamped), yellow at 1-2, white above 2.  Put the frame and its
 * vmap side by side and the question "does this model agree with my eyes"
 * becomes answerable in about four seconds. */
static const char *ct_dump_pfx;

static void ct_vmap(const uint32_t *under, const uint32_t *over,
                    uint32_t *dst, int w, int h)
{
    ct_tiles(under, w, h);
    for (int y = 0; y < h; y++) {
        int ty = y / ct_tsz; if (ty >= ct_nty) ty = ct_nty - 1;
        for (int x = 0; x < w; x++) {
            size_t i = (size_t)y * w + x;
            uint32_t a = under[i], b = over[i];
            int l = (int)(ct_luma(a) * 0.5);
            dst[i] = 0xFF000000u | (uint32_t)l << 16 | (uint32_t)l << 8 | (uint32_t)l;
            if (a == b) continue;
            int ar = a >> 16 & 255, ag = a >> 8 & 255, ab = a & 255;
            int br = b >> 16 & 255, bg = b >> 8 & 255, bb = b & 255;
            double dL  = ((br - ar) * 54 + (bg - ag) * 183 + (bb - ab) * 19) / 256.0;
            double drg = (double)((br - bg) - (ar - ag));
            double dyb = ((br + bg) * 0.5 - bb) - ((ar + ag) * 0.5 - ab);
            double S   = __builtin_sqrt(dL * dL + 0.25 * drg * drg + 0.16 * dyb * dyb);
            if (S < 0.5) continue;
            int tx = x / ct_tsz; if (tx >= ct_ntx) tx = ct_ntx - 1;
            int t  = ty * ct_ntx + tx;
            double V = S / (0.5 * ct_tsd[t] + 0.05 * ct_tm[t] + 2.0);
            /* red -> yellow -> white as V crosses 1 and then 2 */
            int R = 255, G, B;
            if (V < 1.0)      { G = (int)(V * 120);            B = 0; }
            else if (V < 2.0) { G = 120 + (int)((V - 1) * 135); B = 0; }
            else              { G = 255; B = (int)((V - 2) * 160); if (B > 255) B = 255; }
            dst[i] = 0xFF000000u | (uint32_t)R << 16 | (uint32_t)G << 8 | (uint32_t)B;
        }
    }
}

static void ct_dump(const char *tag, const uint32_t *under,
                    const uint32_t *over, int w, int h)
{
    static uint32_t *vm; static int vn;
    char p[512];
    if (vn < w * h) { free(vm); vm = malloc((size_t)w * h * 4); vn = vm ? w * h : 0; }
    snprintf(p, sizeof p, "%s_%s.ppm", ct_dump_pfx, tag);            ppm(p, over, w, h);
    snprintf(p, sizeof p, "%s_%s_under.ppm", ct_dump_pfx, tag);      ppm(p, under, w, h);
    if (!vm) return;
    ct_vmap(under, over, vm, w, h);
    snprintf(p, sizeof p, "%s_%s_vmap.ppm", ct_dump_pfx, tag);       ppm(p, vm, w, h);
}

/* FOG.  Fog is not "dark" and it is not "low contrast" on its own — it is low
 * LOCAL contrast over a BRIGHT base, which is exactly a tile whose RMS
 * contrast is under 0.15 while its mean luma is over 90.  Counting those
 * tiles turns the word into a number between 0 and 1. */
static void ct_fog(const uint32_t *b, int w, int h,
                   double *fog, double *dr, double *lrms, double *mean)
{
    long lh[256]; memset(lh, 0, sizeof lh);
    double sum = 0; long npix = (long)w * h;
    for (long i = 0; i < npix; i++) {
        double l = ct_luma(b[i]); sum += l;
        int q = (int)l; if (q < 0) q = 0; if (q > 255) q = 255; lh[q]++;
    }
    ct_tiles(b, w, h);
    long flat = 0; double rms = 0;
    for (int t = 0; t < ct_nt; t++) {
        double m = ct_tm[t], sd = ct_tsd[t];
        double c = sd / (m > 8.0 ? m : 8.0);
        rms += c;
        if (c < CT_FOGC && m > CT_FOGL) flat++;
    }
    *fog  = (double)flat / (double)ct_nt;
    *lrms = rms / ct_nt;
    *mean = sum / (double)npix;
    *dr   = ct_pct(lh, 256, npix, 0.99, 1.0) - ct_pct(lh, 256, npix, 0.01, 1.0);
}

/* ---- accumulators ---------------------------------------------------------
 * Everything is kept per SERIES: one per slot, plus CT_ALL for the union of
 * the overlays (if a fix moves the accent from slot 2 to slot 3 the per-slot
 * numbers move and the union does not, and that difference is the point). */
typedef struct {
    long   seen, present, visible, clear;
    double s_cov, s_v50, s_v90, s_vf1, s_sgn, s_sig, s_ped, s_head;
} ct_series;

typedef struct {                    /* one tenancy = one routine's whole run  */
    int    open, routine, f_in, miss;
    long   n, present, visible;
    double s_cov, s_v50, s_gl;
} ct_ten;

static ct_series ct_S[CT_NSER];
static ct_ten    ct_T[CT_NSER];
static long ct_gh[256];             /* ground mean luma, one bin per code value */
static long ct_fh[101];             /* fog fraction, 1% bins                    */
static long ct_meas_n, ct_frames, ct_ten_n;
static double ct_s_fog, ct_s_dr, ct_s_lrms, ct_s_fin;
static int  ct_every = 3, ct_warm = 600, ct_seed;
static int  ct_f, ct_active, ct_gluma_now, ct_gnd_rt;
static uint32_t *ct_prev, *ct_gbuf;
static double ct_vbest = -1e9, ct_vworst = 1e9, ct_fogworst = -1e9;
static FILE *ct_fcsv, *ct_tcsv;

static void ct_ten_close(int s)
{
    ct_ten *T = &ct_T[s];
    if (!T->open || T->n < 4) { T->open = 0; return; }
    ct_ten_n++;
    if (ct_tcsv)
        fprintf(ct_tcsv, "%d,%d,%d,%d,%ld,%ld,%.4f,%.3f,%.4f,%.1f\n",
                ct_seed, s, T->routine, T->f_in, T->n, T->present,
                T->s_cov / T->n, T->s_v50 / T->n,
                T->present ? (double)T->visible / (double)T->present : 0.0,
                T->s_gl / T->n);
    T->open = 0;
}

static void ct_note(int s, int routine, const ct_meas *m, int w_now, int blend)
{
    ct_series *S = &ct_S[s];
    ct_ten    *T = &ct_T[s];
    int present = m->cov >= CT_COVMIN;
    int visible = present && m->v50 >= 1.0;
    S->seen++; S->present += present; S->visible += visible;
    S->clear += (present && m->v50 >= 2.0);
    S->s_cov += m->cov;  S->s_v50 += m->v50;  S->s_v90 += m->v90;
    S->s_vf1 += m->vf1;  S->s_sgn += m->sgn;  S->s_sig += m->sig;
    S->s_ped += m->ped;  S->s_head += m->head;

    if (T->open && T->routine != routine) ct_ten_close(s);
    if (!T->open) { memset(T, 0, sizeof *T); T->open = 1;
                    T->routine = routine; T->f_in = ct_f; }
    T->miss = 0; T->n++; T->present += present; T->visible += visible;
    T->s_cov += m->cov; T->s_v50 += m->v50; T->s_gl += ct_gluma_now;

    if (ct_fcsv)
        fprintf(ct_fcsv, "%d,%d,%d,%d,%d,%d,%d,%.5f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f\n",
                ct_seed, ct_f, s, routine, w_now, blend, ct_gluma_now,
                m->cov, m->v50, m->v90, m->vf1, m->sgn, m->sig, m->ped);
}

/* The tap.  Ground first, then one call per overlay in composite order, then
 * the finished frame.  Sampling every EVERY-th frame costs nothing in
 * accuracy: consecutive frames are almost perfectly autocorrelated (a tenancy
 * lasts 500-1600 frames), so the independent information is in the tenancies,
 * not the frames. */
static void ct_tap(int stage, int slot, int routine, int w_now, int blend,
                   const uint32_t *fb, int w, int h)
{
    long npix = (long)w * h;
    if (stage == JD_TAP_GROUND) {
        ct_f++;
        ct_active = (ct_f > ct_warm) && (ct_f % ct_every == 0);
        if (!ct_active) return;
        ct_frames++;
        memcpy(ct_prev, fb, (size_t)npix * 4);
        memcpy(ct_gbuf, fb, (size_t)npix * 4);
        double s = 0; for (long i = 0; i < npix; i++) s += ct_luma(fb[i]);
        ct_gluma_now = (int)(s / npix + 0.5);
        if (ct_gluma_now > 255) ct_gluma_now = 255;
        if (ct_gluma_now < 0)   ct_gluma_now = 0;
        ct_gh[ct_gluma_now]++;
        ct_gnd_rt = routine;
        for (int i = 0; i < CT_NSER; i++) if (ct_T[i].open) ct_T[i].miss++;
        return;
    }
    if (!ct_active) return;
    if (stage == JD_TAP_OVERLAY) {
        ct_meas m;
        ct_measure(ct_prev, fb, w, h, &m);
        ct_note(slot, routine, &m, w_now, blend);
        /* keep the best and worst ACCENT frame this run produced — they
         * cannot be recovered afterwards, see ct_dump */
        if (ct_dump_pfx && slot == CT_ACCENT && m.cov >= CT_COVMIN) {
            if (m.v50 > ct_vbest)  { ct_vbest  = m.v50; ct_dump("best",  ct_prev, fb, w, h); }
            if (m.v50 < ct_vworst) { ct_vworst = m.v50; ct_dump("worst", ct_prev, fb, w, h); }
        }
        memcpy(ct_prev, fb, (size_t)npix * 4);
        return;
    }
    /* JD_TAP_FINAL: ct_prev is the composite as the overlays left it; fb has
     * the boot ramp, the dead-air gain, the audio bloom and the HUD on top.
     * The union of the overlays is measured against the ground on the
     * PRE-gain buffers (gain scales accent and ground alike, so it cannot
     * change V) while fog is measured on the frame the eye is given. */
    { ct_meas m; ct_measure(ct_gbuf, ct_prev, w, h, &m);
      ct_note(CT_ALL, ct_gnd_rt, &m, 0, -1); }
    { double fog, dr, lrms, mean;
      ct_fog(fb, w, h, &fog, &dr, &lrms, &mean);
      ct_s_fog += fog; ct_s_dr += dr; ct_s_lrms += lrms; ct_s_fin += mean;
      int q = (int)(fog * 100.0 + 0.5); if (q > 100) q = 100; ct_fh[q]++;
      if (ct_dump_pfx && fog > ct_fogworst) {
          char p[512]; ct_fogworst = fog;
          snprintf(p, sizeof p, "%s_fog.ppm", ct_dump_pfx); ppm(p, fb, w, h);
      } }
    ct_meas_n++;
    for (int i = 0; i < CT_NSER; i++)
        if (ct_T[i].open && ct_T[i].miss > 20) ct_ten_close(i);
}

static int ct_setup(int w, int h)
{
    ct_tsz = (w < h ? w : h) / 24; if (ct_tsz < 8) ct_tsz = 8; if (ct_tsz > 64) ct_tsz = 64;
    ct_ntx = (w + ct_tsz - 1) / ct_tsz; ct_nty = (h + ct_tsz - 1) / ct_tsz;
    ct_nt  = ct_ntx * ct_nty;
    ct_tm  = malloc((size_t)ct_nt * sizeof *ct_tm);
    ct_tsd = malloc((size_t)ct_nt * sizeof *ct_tsd);
    ct_s1  = malloc((size_t)ct_nt * sizeof *ct_s1);
    ct_s2  = malloc((size_t)ct_nt * sizeof *ct_s2);
    ct_cn  = malloc((size_t)ct_nt * sizeof *ct_cn);
    ct_prev = malloc((size_t)w * h * 4);
    ct_gbuf = malloc((size_t)w * h * 4);
    return ct_tm && ct_tsd && ct_s1 && ct_s2 && ct_cn && ct_prev && ct_gbuf;
}

static void ct_report(void)
{
    static const char *NM[CT_NSER] = { "base", "mid", "ACCENT", "spark", "shadow", "ALL-OV" };
    printf("\n== contrast: seed(start)=%d  frames run=%d  frames measured=%ld"
           "  tiles=%dx%d@%dpx ==\n", ct_seed, ct_f, ct_frames, ct_ntx, ct_nty, ct_tsz);
    printf("%-7s %6s %7s %7s %7s %6s %6s %6s %6s %6s\n",
           "series", "onscr", "present", "VISIBLE", "clear", "cov", "V50", "sig", "sigma", "ped");
    for (int s = 0; s < CT_NSER; s++) {
        ct_series *S = &ct_S[s];
        if (!S->seen) { printf("%-7s      -\n", NM[s]); continue; }
        double n = (double)S->seen, p = (double)S->present;
        printf("%-7s %6ld %6.1f%% %6.1f%% %6.1f%% %5.1f%% %6.2f %6.1f %6.1f %6.1f\n",
               NM[s], S->seen, 100.0 * p / n,
               p ? 100.0 * (double)S->visible / p : 0.0,
               p ? 100.0 * (double)S->clear   / p : 0.0,
               100.0 * S->s_cov / n, S->s_v50 / n, S->s_sgn / n,
               S->s_sig / n, S->s_ped / n);
    }
    /* ground: how often is it bright enough to swamp anything put on it? */
    long gt = 0; for (int i = 0; i < 256; i++) gt += ct_gh[i];
    long bright = 0, dark = 0;
    for (int i = 141; i < 256; i++) bright += ct_gh[i];
    for (int i = 0;   i <  40; i++) dark   += ct_gh[i];
    printf("\nground luma  p10=%.0f p50=%.0f p90=%.0f   >140: %.1f%% of frames"
           "   <40: %.1f%%   mean headroom under the accent %.0f%%\n",
           ct_pct(ct_gh, 256, gt, 0.10, 1.0), ct_pct(ct_gh, 256, gt, 0.50, 1.0),
           ct_pct(ct_gh, 256, gt, 0.90, 1.0),
           gt ? 100.0 * bright / gt : 0.0, gt ? 100.0 * dark / gt : 0.0,
           ct_S[CT_ACCENT].seen ? 100.0 * ct_S[CT_ACCENT].s_head / ct_S[CT_ACCENT].seen : 0.0);
    long ft = 0; for (int i = 0; i <= 100; i++) ft += ct_fh[i];
    double m = ct_meas_n ? (double)ct_meas_n : 1.0;
    printf("fog          p50=%.2f p90=%.2f mean=%.2f | local RMS contrast %.3f"
           " | dyn range %.0f/255 | final luma %.1f\n",
           ct_pct(ct_fh, 101, ft, 0.50, 100.0), ct_pct(ct_fh, 101, ft, 0.90, 100.0),
           ct_s_fog / m, ct_s_lrms / m, ct_s_dr / m, ct_s_fin / m);
    if (ct_dump_pfx)
        printf("dumped       %s_best*.ppm (V50 %.2f)  %s_worst*.ppm (V50 %.2f)"
               "  %s_fog.ppm (fog %.2f)\n",
               ct_dump_pfx, ct_vbest, ct_dump_pfx, ct_vworst, ct_dump_pfx, ct_fogworst);
    /* the one line a script should parse */
    { ct_series *A = &ct_S[CT_ACCENT], *U = &ct_S[CT_ALL];
      double ap = A->seen ? (double)A->present / A->seen : 0.0;
      double av = A->present ? (double)A->visible / A->present : 0.0;
      double uv = U->present ? (double)U->visible / U->present : 0.0;
      printf("CONTRAST seed=%d meas=%ld ten=%ld acc_present=%.4f acc_visible=%.4f"
             " acc_v50=%.4f all_visible=%.4f fog=%.4f lrms=%.4f dr=%.1f"
             " gluma=%.1f finluma=%.1f\n",
             ct_seed, ct_meas_n, ct_ten_n, ap, av,
             A->seen ? A->s_v50 / A->seen : 0.0, uv,
             ct_s_fog / m, ct_s_lrms / m, ct_s_dr / m,
             ct_pct(ct_gh, 256, gt, 0.50, 1.0), ct_s_fin / m); }
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
    if (!strcmp(cmd, "keys")) {
        /* Three questions, and the third is the one that matters:
         *   1. did C actually change the colour?      (the ramp itself)
         *   2. did S actually change the shapes?      (jd_now_playing)
         *   3. did either of them strobe?             (single-frame delta)
         * The engine's one law is that nothing strobes, and a key press is
         * not a licence to break it — so the delta after a press is measured
         * against the delta the same run produces on its own. */
        int start = atoi(argv[4]), total = atoi(argv[5]);
        /* how long to watch after a press.  Departures here are envelopes,
         * not cuts — the ground alone takes about three seconds to hand over
         * — so a short window measures the fade, not the outcome. */
        #define KWIN 300
        int f_c = total / 3, f_s = (total * 2) / 3;
        uint32_t pal_before[8], pal_after[8];
        jd_nowplaying np_before[8], np_after[8], np_late[8];
        int nb = 0, na = 0, nl = 0;
        double d_base = 0, d_c = 0, d_s = 0;      /* worst delta per window  */
        int    n_base = 0;
        jd_frame(fb, w, h, start);
        for (int f = 1; f < total; f++) {
            int gf = start + f;
            /* sample the ramp and the cast just before each press */
            if (f == f_c) {
                const uint32_t *r = jd_blend_ramp();
                for (int i = 0; i < 8; i++) pal_before[i] = r[i * 4096];
                ppm("/tmp/jd_key_c_before.ppm", fb, w, h);
                jd_req_palette = 1;
            }
            if (f == f_s) {
                nb = jd_now_playing(np_before, 8);
                ppm("/tmp/jd_key_s_before.ppm", fb, w, h);
                jd_req_shape = 1;
            }

            memcpy(prev, fb, (size_t)n * 4);
            jd_frame(fb, w, h, gf);
            double d = delta_of(prev, fb, n);

            /* windows: 60 frames after each press, everything else is the
             * baseline this run makes on its own */
            if      (f > f_c && f <= f_c + KWIN) { if (d > d_c) d_c = d; }
            else if (f > f_s && f <= f_s + KWIN) { if (d > d_s) d_s = d; }
            else { if (d > d_base) d_base = d; n_base++; }

            if (f == f_c + 90) ppm("/tmp/jd_key_c_after.ppm", fb, w, h);
            if (f == f_c + KWIN) {
                const uint32_t *r = jd_blend_ramp();
                for (int i = 0; i < 8; i++) pal_after[i] = r[i * 4096];
            }
            if (f == f_s + KWIN) { na = jd_now_playing(np_after, 8);
                                   ppm("/tmp/jd_key_s_after.ppm", fb, w, h); }
            if (f == f_s + KWIN*2) nl = jd_now_playing(np_late, 8);
        }
        /* 1. colour: mean per-channel distance across 8 taps of the ramp */
        double pd = 0;
        for (int i = 0; i < 8; i++) {
            uint32_t a = pal_before[i], b = pal_after[i];
            pd += abs((int)(a >> 16 & 255) - (int)(b >> 16 & 255))
                + abs((int)(a >> 8  & 255) - (int)(b >> 8  & 255))
                + abs((int)(a       & 255) - (int)(b       & 255));
        }
        pd /= 24.0;
        printf("C  palette moved %.1f/255 per channel\n", pd);
        /* 2. shapes: how much of the cast turned over */
        int held = 0, held_l = 0;
        for (int i = 0; i < na; i++)
            for (int j = 0; j < nb; j++)
                if (np_after[i].routine == np_before[j].routine) { held++; break; }
        for (int i = 0; i < nl; i++)
            for (int j = 0; j < nb; j++)
                if (np_late[i].routine == np_before[j].routine) { held_l++; break; }
        printf("S  cast %d -> %d, %d held over (at +%d: %d of %d held)\n",
               nb, na, held, KWIN * 2, held_l, nl);
        static const char *RL[] = { "GROUND", "FIELD", "FIGURE", "SPARK" };
        for (int i = 0; i < nb; i++)
            printf("   was %-6s %s\n", RL[np_before[i].role & 3], jd_routine_name(np_before[i].routine));
        for (int i = 0; i < na; i++)
            printf("   now %-6s %s\n", RL[np_after[i].role & 3], jd_routine_name(np_after[i].routine));
        /* 3. strobe */
        printf("DELTA baseline max %.2f (%d frames) | after C %.2f | after S %.2f\n",
               d_base, n_base, d_c, d_s);
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
    if (!strcmp(cmd, "contrast")) {
        /* Warm-up is discarded, not measured: the boot ramp, the opening
         * emblem, the first-run card and the 4-slot cascade all land inside
         * the first few hundred frames and none of them is the show. */
        if (argc < 6) { fprintf(stderr, "usage: contrast W H START TOTAL [EVERY] [f.csv] [t.csv]\n"); return 2; }
        int start = atoi(argv[4]), total = atoi(argv[5]);
        if (argc > 6) ct_every = atoi(argv[6]);
        if (ct_every < 1) ct_every = 1;
        { const char *e = getenv("JD_CT_WARM"); if (e) ct_warm = atoi(e); }
        ct_dump_pfx = getenv("JD_CT_DUMP");
        if (!ct_setup(w, h)) { fprintf(stderr, "contrast: out of memory\n"); return 2; }
        if (argc > 7 && strcmp(argv[7], "-")) {
            ct_fcsv = fopen(argv[7], "w");
            if (ct_fcsv) fprintf(ct_fcsv, "seed,frame,slot,routine,w,blend,gluma,"
                                          "cov,v50,v90,vf1,signal,sigma,pedestal\n");
        }
        if (argc > 8 && strcmp(argv[8], "-")) {
            ct_tcsv = fopen(argv[8], "w");
            if (ct_tcsv) fprintf(ct_tcsv, "seed,slot,routine,f_in,samples,present,"
                                          "cov,v50,vis_frac,gluma\n");
        }
        ct_seed = start;
        jd_tap = ct_tap;
        for (int f = 0; f < total; f++) jd_frame(fb, w, h, start + f);
        jd_tap = NULL;
        for (int i = 0; i < CT_NSER; i++) ct_ten_close(i);   /* flush the open ones */
        if (ct_fcsv) fclose(ct_fcsv);
        if (ct_tcsv) fclose(ct_tcsv);
        ct_report();
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
