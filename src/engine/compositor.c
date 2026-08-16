/* ============================================================
 * bridge.c — JellyDazzle v2.1 dispatcher / layer compositor
 *
 * v2.0 rolled one die per 2048-frame segment: mix32(seg) % 124 picked a
 * routine, two more dice picked a palette pair, and the screen hard-cut at
 * every seam.  Measured: 110 of 124 routines in 300 segments, 39 repeats
 * inside any 20-segment window, palette usage 5..16, 95.5 mean channel
 * delta at the cut.
 *
 * v2.1 replaces all of that with:
 *   1. shuffled BAGS (per role for routines, per epoch for palettes) so
 *      every member plays once before any repeat  — lab/design/scheduler.md
 *   2. a LAYER COMPOSITOR: 4 independently-clocked slots (base / mid /
 *      accent / spark) that enter 0s, +3s, +4s, +9s and each live on their
 *      own envelope                              — lab/design/compositor.md
 *   3. crossfades everywhere — layer envelopes are smootherstep, the base
 *      hands over THROUGH its successor, and the palette walk is chained
 *      and eased so no leg seam has a value or a velocity step
 *                                                — lab/design/transitions.md
 *   4. per-layer palette windows + a per-tenancy "mood" so the same pool
 *      reads stark one minute and blazing the next.
 *
 * Everything the scheduler needs to know about a routine (role, motion,
 * cost, darkness) is MEASURED at startup by probing each pattern at two
 * resolutions — so a doubled pattern library schedules itself correctly
 * with no table to regenerate.  Cost is then tracked by an EWMA of the
 * real render at the real resolution.
 *
 * JD_MODE=N still forces one routine, no layers, exactly as in v2.0.
 * Other env knobs:  JD_LAYERS=n (cap slots 1..4)   JD_NOPROBE=1
 * ============================================================ */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include "jellydazzle.h"

/* JD_DEBUG=1 traces spawns/retires and per-frame occupancy on stderr */
#ifndef JD_TRACE
#define JD_TRACE 1
#endif
#if JD_TRACE
#include <stdio.h>
static int g_dbg = 0;                      /* 1 = events, 2 = every frame */
#define TR(...)  do { if (g_dbg)      fprintf(stderr, __VA_ARGS__); } while (0)
#define TRF(...) do { if (g_dbg >= 2) fprintf(stderr, __VA_ARGS__); } while (0)
#else
#define TR(...)  do { } while (0)
#define TRF(...) do { } while (0)
#endif

/* number of palette schemes in jd_palette.  The Makefile derives this from
 * palette.bin so doubling the palettes needs no edit here. */
#ifndef JD_NS
#define JD_NS 30
#endif

uint32_t g_mode = 0;                       /* read by draw.s mode select */
extern void draw_frame(uint32_t*, int, int, int);
extern const uint32_t jd_palette[];        /* JD_NS*32768 ARGB, in draw.s */
extern const jd_pattern_fn jd_patterns[];  /* registry.c, indexed 0..N-1 */
extern const int jd_pattern_count;

/* ---------------- knobs ---------------- */
#define JD_NASM        24                  /* asm modes 0..23             */
#define JD_ASM_ACC0    15                  /* asm 15..23 own their canvas */
#define JD_MAXR        640                 /* routine table capacity      */
#define JD_NSLOT        4                  /* base, mid, accent, spark    */
#define JD_LUMA_FLOOR   30                 /* dead-air only, not 'dim'    */
#define JD_LUMA_MAXGAIN 384                /* <= 1.5x: 2.7x clamped the
                                            * highlights and washed the
                                            * colour out (J caught it)   */
#define JD_SHADOW       4                  /* base successor (handover)   */
#define JD_BOOT       120                  /* launch fade-up, frames      */
#define JD_NBUF         5
#define PAL_N       32768
#define PAL_MASK    0x7FFF
#define PAL_RET     2048
#define JD_LEG       1024                  /* frames per palette leg      */
#define JD_SEAM        16                  /* bag anti-repeat window      */
#define JD_MAXSL     2000                  /* cap on layer-local sl       */

#define BUDGET_Q8    (10*256+128)          /* 10.5 ms of render+blend     */
#define DCAP_Q8      (6*256 + 128)         /* 6.5 weighted motion units   */
#define FREEZE_W        38                 /* Q8: skip render below 0.15  */

enum { R_GROUND = 0, R_FIELD, R_FIGURE, R_SPARK, R_NROLE };
enum { C_PURE = 0, C_CANVAS };             /* canvas-owning = asm 15..23  */
enum { B_MIX = 0, B_MAX, B_SCREEN, B_ADD, B_DIFF };
enum { M_STARK = 0, M_RICH, M_BLAZE, M_N };

/* ---------------- small maths ---------------- */
static uint32_t mix32(uint32_t x) {
    x *= 0x9E3779B1u; x ^= x >> 16;
    x *= 0x85EBCA6Bu; x ^= x >> 13;
    return x;
}

/* smootherstep, Q16 -> Q16, factored + rounded (the naive expansion is
 * non-monotonic near the top and produces a one-frame un-fade). */
static uint32_t ease_ss(uint32_t x) {
    if (x >= 65536u) return 65536u;
    uint64_t X = x;
    uint64_t x2 = (X * X + 32768) >> 16;
    uint64_t x3 = (x2 * X + 32768) >> 16;
    int64_t  in = 10 * 65536 - 15 * (int64_t)X + 6 * (int64_t)x2;
    int64_t  r  = ((int64_t)x3 * in + 32768) >> 16;
    return (uint32_t)(r < 0 ? 0 : (r > 65536 ? 65536 : r));
}

static double now_ms(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static void probe_cache_save(void);

/* ---------------- routine statistics ---------------- */
typedef struct {
    uint8_t  role;      /* R_*                                        */
    uint8_t  cls;       /* C_PURE | C_CANVAS                          */
    uint8_t  dark;      /* fraction of near-black pixels, /255        */
    uint8_t  luma;      /* mean luma 0..255                           */
    uint8_t  sat;       /* mean channel spread 0..255                 */
    uint8_t  probed;
    uint16_t delta_q8;  /* frame-to-frame mean channel delta, Q8      */
    uint16_t cost_q8;   /* render ms at full res, Q8 (EWMA at run)    */
} jd_stat;

static jd_stat g_st[JD_MAXR];
static int     g_nr = 0;                   /* 24 + jd_pattern_count      */

/* ---------------- shuffled bags ---------------- */
typedef struct {
    uint16_t items[JD_MAXR];
    uint16_t buf[JD_MAXR];
    uint16_t hist[JD_SEAM];
    uint16_t n, head;
    uint8_t  hpos;
    uint32_t seed;
} jd_bag;

static jd_bag g_bag[R_NROLE];

/* ---------------- layers ---------------- */
typedef struct {
    int       live;
    int       routine;
    int       t_in, t_full, t_out, t_end;
    int       sl;                 /* layer-local, +1 per RENDER          */
    int       fbase;              /* animation clock origin, kept small  */
    uint32_t  seed;
    uint16_t  w_peak;             /* Q8 0..256                           */
    uint16_t  mdel;               /* measured motion of THIS tenancy, Q8 */
    uint16_t  w_now;              /* Q8 this frame                       */
    uint32_t  w16;                /* Q16 this frame — ground ratio needs it */
    uint8_t   blend;
    uint8_t   cls;
    uint8_t   half, parity;       /* half-rate decimation (fallback)     */
    uint8_t   frozen;             /* strobed: stop calling it, dissolve  */
    uint8_t   strikes;
    uint32_t  span, off;          /* palette window                      */
    float     lo_s, hi_s, gs_s;   /* reshape params, fixed at spawn      */
    uint8_t   lut[256];           /* value transfer curve, built once    */
    uint32_t  gq;                 /* saturation gain, Q8                 */
    uint32_t *buf;
    uint32_t *pal;
    double    cost_ms;
} jd_layer;

static int       g_last_change = -100000;  /* frame of the last spawn      */
static int       g_gap   = 60;   /* min frames between entries, re-rolled per spawn */
static uint32_t  g_tempo = 256;  /* Q8 pace of the stack under this ground: 0.7..1.33 */
static int       g_lead  = 120;  /* frames before t_out the ground's successor starts */
static jd_layer  g_L[JD_NBUF];
static uint32_t *g_buf[JD_NBUF];
static uint32_t *g_pal[JD_NBUF];
static int       g_rest[JD_NBUF];          /* frame at which slot may spawn */

/* ---------------- palette walk ---------------- */
static uint32_t  g_blend[PAL_N];           /* shared scheme-blended ramp  */
static int       g_blend_key = -1;
static int       g_ns = JD_NS;
static uint8_t   g_pbag[2][256];
static uint32_t  g_pepoch[2] = { 0xFFFFFFFFu, 0xFFFFFFFFu };
static float     g_pfeat[256][14];         /* hue12 + sat + val           */
static float     g_pthresh = 0.0f;

/* ---------------- engine state ---------------- */
static int    g_w = 0, g_h = 0;
static int    g_ready = 0;
static int    g_frame0 = 0;
/* Per-RUN entropy.  main.c starts the frame counter at a random value each
 * launch, but the v2.1 scheduler draws from shuffled BAGS instead of hashing
 * the frame — so that randomness stopped reaching the deck order and every
 * launch dealt the same cards (J: "same patterns every time").  Every shuffle
 * mixes this in. */
static uint32_t g_run = 0;
static uint32_t g_gain = 256;              /* dead-air lift, Q8            */
static uint32_t g_prot = 0;       /* audio palette rotation, 0..PAL_N-1 */
void jd_audio_meter_draw(uint32_t *fb, int w, int h);   /* AUDIO: HUD, src/audio/listen.c */
static int    g_mood = M_RICH;
static int    g_prev_mood = M_RICH;
static double g_ewma_ms = 6.0;
static int    g_hot = 0, g_cool = 0, g_jitter = 0;
static uint32_t g_sig[1024];
static uint32_t g_lsig[JD_NBUF][512];      /* per-layer motion signature   */
static uint8_t  g_lsig_ok[JD_NBUF];
static int      g_sig_n = 0;
static double   g_motion = 0.0;            /* EWMA composite delta        */
static int      g_slot_cap = JD_NSLOT;

/* ============================================================
 * 1. statistics: probe every pattern at two resolutions
 * ============================================================ */

static void stat_image(const uint32_t *b, int n, jd_stat *s)
{
    uint64_t lum = 0, sat = 0; uint32_t dark = 0; int cnt = 0;
    for (int i = 0; i < n; i += 3) {
        uint32_t c = b[i];
        uint32_t r = (c >> 16) & 255, g = (c >> 8) & 255, bl = c & 255;
        uint32_t l = (r * 77 + g * 150 + bl * 29) >> 8;
        uint32_t mx = r > g ? (r > bl ? r : bl) : (g > bl ? g : bl);
        uint32_t mn = r < g ? (r < bl ? r : bl) : (g < bl ? g : bl);
        lum += l; sat += mx - mn; if (l < 16) dark++;
        cnt++;
    }
    if (!cnt) cnt = 1;
    s->luma = (uint8_t)(lum / cnt);
    s->sat  = (uint8_t)(sat / cnt);
    s->dark = (uint8_t)((uint64_t)dark * 255 / cnt);
}

static uint32_t delta_q8_of(const uint32_t *a, const uint32_t *b, int n)
{
    uint64_t s = 0; int cnt = 0;
    for (int i = 0; i < n; i += 3) {
        uint32_t p = a[i], q = b[i];
        int dr = (int)((p >> 16) & 255) - (int)((q >> 16) & 255);
        int dg = (int)((p >>  8) & 255) - (int)((q >>  8) & 255);
        int db = (int)( p        & 255) - (int)( q        & 255);
        s += (uint32_t)(dr < 0 ? -dr : dr) + (uint32_t)(dg < 0 ? -dg : dg)
           + (uint32_t)(db < 0 ? -db : db);
        cnt += 3;
    }
    if (!cnt) cnt = 1;
    return (uint32_t)(s * 256 / cnt);
}

static void role_from_cov(jd_stat *s)
{
    int cov = 255 - s->dark;                     /* 0..255 */
    s->role = cov >= 204 ? R_GROUND : cov >= 115 ? R_FIELD
            : cov >=  38 ? R_FIGURE : R_SPARK;

    /* Coverage is not brightness.  A routine can fill the frame with
     * near-black and still measure as full coverage; promoting one of those
     * to GROUND puts a dead screen up for a whole tenancy (measured: 14 of
     * 40 samples under luma 40, one at 0).  A ground layer carries the
     * picture, so require it to actually emit light; dim material is still
     * welcome as an overlay, where whatever is underneath shows through. */
    if (s->role == R_GROUND && s->luma < 55) s->role = R_FIELD;
    if (s->role == R_FIELD  && s->luma < 28) s->role = R_FIGURE;
}

/* Probe one pattern: PW x PH for shape/motion, then 2x for the cost model.
 * cost(A) = fixed + slope*A, solved from the two measurements, evaluated at
 * the real framebuffer area.  Returns ms spent. */
#define PW 320
#define PH 240

static void render_one(int rt, uint32_t *dst, int w, int h, int frame, int sl,
                       uint32_t seed, const uint32_t *pal)
{
    if (rt < JD_NASM) { g_mode = (uint32_t)rt; draw_frame(dst, w, h, frame); }
    else              jd_patterns[rt - JD_NASM](dst, w, h, frame, sl, seed, pal);
}

/* The probe is RESUMABLE: one render per call, state carried in the pr_*
 * statics.  Measured, a whole routine costs 8..87 ms (median ~35), so
 * running routines to completion inside a frame overran the 2.5 ms budget
 * by 10-35x and put ~250 frames of 20..300 ms into the opening fade — the
 * single worst hitch left in the engine.  Splitting it at the render
 * boundary caps the overrun at one sub-frame (~0.5-6 ms) and changes
 * nothing about the arithmetic: every expression below is the original,
 * only its home moved, and the render order and inputs are identical. */
static int      pr_rt = -1;         /* routine in flight, -1 = between      */
static int      pr_phase = 0;       /* 0 = 320x240 motion, 1 = 640x480 cost */
static int      pr_f = 0;           /* sub-frame within the phase           */
static uint32_t pr_seed, pr_dsum, pr_dmax;
static int      pr_dn;
static double   pr_tsmall;

static void probe_open(int rt, uint32_t *a, uint32_t *b)
{
    pr_rt = rt; pr_phase = 0; pr_f = 0;
    pr_seed = mix32(0xC0FFEE11u + (uint32_t)rt + g_run);
    pr_dsum = 0; pr_dmax = 0; pr_dn = 0; pr_tsmall = 0.0;
    memset(a, 0, (size_t)PW * PH * 4);
    memset(b, 0, (size_t)PW * PH * 4);
}

/* Advance the routine in flight by ONE render.  Returns 1 when it is done. */
static int probe_advance(uint32_t *a, uint32_t *b, const uint32_t *pal, int fullpix)
{
    int rt = pr_rt;
    jd_stat *s = &g_st[rt];

    if (pr_phase == 0) {
        /* Motion pass.  One or two samples are not enough: routines that
         * start still and wind up read as calm, accumulators priming read
         * as strobes, and — the case that actually got through — a routine
         * that hard-cuts its whole composition every N frames looks
         * perfectly smooth in between.  So run 48 frames and keep both the
         * average and the worst single step. */
        int f = pr_f++;
        uint32_t *dst = (f & 1) ? b : a;
        const uint32_t *prv = (f & 1) ? a : b;
        memcpy(dst, prv, (size_t)PW * PH * 4);      /* canvas continuity */
        double m0 = (f == 40) ? now_ms() : 0.0;
        render_one(rt, dst, PW, PH, f, f, pr_seed, pal);
        if (f == 40) pr_tsmall = now_ms() - m0;
        if (f >= 4) {
            uint32_t d = delta_q8_of(dst, prv, PW * PH);
            pr_dsum += d; pr_dn++;
            if (d > pr_dmax) pr_dmax = d;
        }
        if (pr_f < 48) return 0;

        stat_image(b, PW * PH, s);   /* frame 47 landed in b */
        role_from_cov(s);
        uint32_t dmean = pr_dn ? pr_dsum / pr_dn : 0;
        /* the probe runs at 320x240; fine detail that aliases away there
         * still moves at 1280x960, so charge ~1.6x.  And a routine that
         * lurches once a second is a strobe even if its average is calm — a
         * quarter of the worst step is enough to keep it out of a busy
         * stack. */
        uint32_t dm = dmean * 8 / 5;
        if (pr_dmax / 4 > dm) dm = pr_dmax / 4;
        s->delta_q8 = (uint16_t)(dm > 65000 ? 65000 : dm);

        memset(a, 0, (size_t)PW * PH * 16);
        pr_phase = 1; pr_f = 0;
        return 0;
    }

    /* Cost pass: two warm-ups at 2x, then one timed frame. */
    {
        int f = pr_f++;
        double m1 = (f == 2) ? now_ms() : 0.0;
        render_one(rt, a, PW * 2, PH * 2, f, f, pr_seed, pal);
        if (f < 2) return 0;
        double tbig = now_ms() - m1;

        double a1 = PW * PH, a2 = PW * PH * 4.0;
        double slope = (tbig - pr_tsmall) / (a2 - a1);
        double fixed = pr_tsmall - slope * a1;
        double full  = fixed + slope * fullpix;
        if (slope < 0.0 || full < tbig) full = tbig * 1.2;
        full *= 1.15;                       /* margin: the model under-reads */
        if (full < 0.10) full = 0.10;
        if (full > 24.0) full = 24.0;
        s->cost_q8 = (uint16_t)(full * 256.0);
        s->probed  = 1;
        if (rt < JD_NASM) {
            /* asm modes read jd_palette directly and take no layer palette,
             * so they can only ever be grounds whatever coverage says */
            s->role = R_GROUND;
            s->cls  = rt >= JD_ASM_ACC0 ? C_CANVAS : C_PURE;
        } else {
            s->cls = C_PURE;
        }
        return 1;
    }
}

static void stats_defaults(void)
{
    /* asm modes: role is forced (they read jd_palette directly and can only
     * be grounds); the rest is measured by the probe like everything else. */
    for (int m = 0; m < JD_NASM; m++) {
        jd_stat *s = &g_st[m];
        s->role = R_GROUND; s->probed = 0;
        if (m >= JD_ASM_ACC0) {
            s->cls = C_CANVAS; s->cost_q8 = (uint16_t)(0.05 * 256);
            s->delta_q8 = 256; s->dark = 150; s->luma = 40; s->sat = 60;
        } else {
            s->cls = C_PURE;  s->cost_q8 = (uint16_t)(7.0 * 256);
            s->delta_q8 = (uint16_t)(2.5 * 256);
            s->dark = 20; s->luma = 120; s->sat = 90;
        }
    }
    for (int i = JD_NASM; i < g_nr; i++) {
        jd_stat *s = &g_st[i];
        s->role = R_FIGURE; s->cls = C_PURE;
        s->cost_q8 = (uint16_t)(3.0 * 256);
        s->delta_q8 = (uint16_t)(1.5 * 256);
        s->dark = 128; s->luma = 90; s->sat = 80; s->probed = 0;
    }
}

/* Probing every routine at two resolutions costs ~10 ms per routine, which
 * would be a second of black screen at startup and worse as the library
 * grows.  Spend it a few ms at a time instead: the engine runs from frame
 * one on whatever has been measured so far, and the bags are rebuilt when
 * the sweep finishes. */
static uint32_t *g_pa = NULL, *g_pb = NULL;
static uint16_t  g_pdefer[JD_MAXR];        /* live routines the sweep skipped */
static int       g_pdefer_n = 0;
static int       g_probe_i = 0, g_probe_done = 0, g_probe_pix = 0;
static double    g_probe_ms = 0.0;

static void probe_begin(int fullpix)
{
    g_probe_pix = fullpix; pr_rt = -1; g_pdefer_n = 0;
    if (getenv("JD_NOPROBE") || getenv("JD_MODE")) { g_probe_done = 1; return; }
    g_pa = (uint32_t*)malloc((size_t)PW * PH * 4 * 4);
    g_pb = (uint32_t*)malloc((size_t)PW * PH * 4);
    if (!g_pa || !g_pb) { free(g_pa); free(g_pb); g_pa = g_pb = NULL; g_probe_done = 1; }
}

/* routines currently on screen: the probe must never render one of these */
static int routine_live(int rt)
{
    for (int i = 0; i < JD_NBUF; i++)
        if (g_L[i].live && g_L[i].routine == rt) return 1;
    return 0;
}

static void stat_from_live(int rt)
{
    for (int i = 0; i < JD_NBUF; i++) {
        jd_layer *L = &g_L[i];
        if (!L->live || L->routine != rt) continue;
        jd_stat *s = &g_st[rt];
        stat_image(L->buf, g_w * g_h, s);
        role_from_cov(s);
        if (L->sl > 60 && L->mdel) s->delta_q8 = L->mdel;
        double c = L->cost_ms * 1.15; if (c < 0.10) c = 0.10; if (c > 24.0) c = 24.0;
        s->cost_q8 = (uint16_t)(c * 256.0);
        if (rt < JD_NASM) { s->role = R_GROUND; s->cls = rt >= JD_ASM_ACC0 ? C_CANVAS : C_PURE; }
        else s->cls = C_PURE;
        s->probed = 1;
        TR("PROBE live-stat rt=%d role=%d luma=%d dark=%d\n", rt, s->role, s->luma, s->dark);
        return;
    }
}

/* returns 1 on the step that finishes the sweep */
static int probe_step(double budget_ms)
{
    if (g_probe_done) return 0;
    static int nfix = -2;
    if (nfix == -2) { const char *e = getenv("JD_PROBEN"); nfix = e ? atoi(e) : -1; }
    int nleft = nfix;
    double t0 = now_ms();
    for (;;) {
        if (pr_rt < 0) {
            int rt;
            if (g_probe_i < g_nr) {
                int np = g_nr - JD_NASM;
                int rot = (int)(g_run % (uint32_t)(np > 0 ? np : 1));
                rt = g_probe_i < np ? JD_NASM + (g_probe_i + rot) % np : g_probe_i - np;
                g_probe_i++;
                if (routine_live(rt)) {
                    if (g_pdefer_n < JD_MAXR) g_pdefer[g_pdefer_n++] = (uint16_t)rt;
                    continue;
                }
            } else {
                int k = g_pdefer_n - 1;
                while (k >= 0 && routine_live(g_pdefer[k])) {
                    int live_sl = 0;
                    for (int i = 0; i < JD_NBUF; i++)
                        if (g_L[i].live && g_L[i].routine == g_pdefer[k]) live_sl = g_L[i].sl;
                    if (live_sl >= 48) {
                        stat_from_live(g_pdefer[k]);
                        g_pdefer[k] = g_pdefer[--g_pdefer_n]; k = g_pdefer_n - 1;
                    } else k--;
                }
                if (k < 0) break;
                rt = g_pdefer[k]; g_pdefer[k] = g_pdefer[--g_pdefer_n];
            }
            probe_open(rt, g_pa, g_pb);
        }
        if (probe_advance(g_pa, g_pb, jd_palette, g_probe_pix)) pr_rt = -1;
        if (nfix >= 0) { if (--nleft <= 0) break; }
        else if (now_ms() - t0 >= budget_ms) break;
    }
    g_probe_ms += now_ms() - t0;
    if (g_probe_i < g_nr || pr_rt >= 0 || g_pdefer_n) return 0;
    free(g_pa); free(g_pb); g_pa = g_pb = NULL;
    g_probe_done = 1;
    TR("PROBE complete: %d routines, %.0f ms total, bags=%u/%u/%u/%u\n",
       g_nr, g_probe_ms, g_bag[R_GROUND].n, g_bag[R_FIELD].n,
       g_bag[R_FIGURE].n, g_bag[R_SPARK].n);
    probe_cache_save();           /* next launch opens with the full library */
    return 1;
}

/* ============================================================
 * 2. bags
 * ============================================================ */

static int bag_recent(const jd_bag *b, uint16_t v, int win)
{
    for (int i = 0; i < win && i < JD_SEAM; i++) {
        int p = ((int)b->hpos - 1 - i) & (JD_SEAM - 1);
        if (b->hist[p] == v) return 1;
    }
    return 0;
}

static void bag_refill(jd_bag *b)
{
    b->seed = mix32(b->seed ^ 0x9E3779B9u);
    uint32_t r = b->seed;
    for (uint16_t i = 0; i < b->n; i++) b->buf[i] = b->items[i];
    for (uint16_t i = b->n; i > 1; i--) {
        r = mix32(r);
        uint16_t j = (uint16_t)(r % i);
        uint16_t t = b->buf[i - 1]; b->buf[i - 1] = b->buf[j]; b->buf[j] = t;
    }
    uint16_t W = (uint16_t)(b->n / 2 < JD_SEAM ? b->n / 2 : JD_SEAM);
    for (uint16_t i = 0; i < W; i++) {
        for (int g = 0; g < 64 && bag_recent(b, b->buf[i], W); g++) {
            r = mix32(r);
            uint16_t j = (uint16_t)(W + r % (uint32_t)(b->n - W));
            uint16_t t = b->buf[i]; b->buf[i] = b->buf[j]; b->buf[j] = t;
        }
    }
    b->head = 0;
}

/* Draw the next admissible id.  A reject is rotated to the BACK of the
 * current cycle, never re-rolled — that is what keeps the permutation (and
 * therefore the once-per-cycle guarantee) intact under admission control. */
static uint16_t bag_draw(jd_bag *b, int (*ok)(uint16_t, int), int slot)
{
    if (!b->n) return 0xFFFF;
    if (b->head >= b->n) bag_refill(b);
    uint16_t left = (uint16_t)(b->n - b->head);
    for (uint16_t tried = 0; tried < left; tried++) {
        uint16_t v = b->buf[b->head];
        if (ok(v, slot)) {
            b->head++;                       /* consumed, stays consumed */
            b->hist[b->hpos & (JD_SEAM - 1)] = v; b->hpos++;
            return v;
        }
        /* refused: rotate it to the back of the REMAINING cycle and leave
         * head where it is, so it still plays exactly once this cycle —
         * just later, when the stack around it has changed */
        for (uint16_t i = b->head; i + 1 < b->n; i++) b->buf[i] = b->buf[i + 1];
        b->buf[b->n - 1] = v;
    }
    return 0xFFFF;
}

/* ---------------- probe cache -------------------------------------------
 * The probe needs ~6.5 s to measure every routine.  Until it finishes the
 * role bags hold almost nothing — GROUND is just the 24 asm modes, FIELD is
 * empty — so every launch opened with the same narrow slice of the library.
 * Measuring is deterministic for a given build, so we do it once and keep the
 * answer next to the app: subsequent launches start with all 225 routines
 * already sorted into their layers.  A build stamp invalidates the file when
 * the pattern set changes, so a stale cache can never mis-sort anything. */
#define JD_CACHE_MAGIC 0x4A44504Bu       /* 'JDPK' */

static const char *probe_cache_path(void)
{
    static char p[1024];
    const char *home = getenv("HOME");
    if (!home) return NULL;
    snprintf(p, sizeof p, "%s/Library/Application Support/JellyDazzle", home);
    mkdir(p, 0755);                       /* ok if it already exists */
    snprintf(p, sizeof p, "%s/Library/Application Support/JellyDazzle/probe.bin", home);
    return p;
}

/* stamp = magic + routine count + version string, so any change to the
 * library or the build invalidates the cached measurements. */
static uint32_t probe_stamp(void)
{
    uint32_t h = mix32(JD_CACHE_MAGIC ^ (uint32_t)g_nr);
    const char *v = JD_VERSION;
    while (*v) h = mix32(h ^ (uint32_t)(unsigned char)*v++);
    return h;
}

static int probe_cache_load(void)
{
    if (getenv("JD_NOCACHE")) return 0;
    const char *p = probe_cache_path();
    if (!p) return 0;
    FILE *f = fopen(p, "rb");
    if (!f) return 0;
    uint32_t magic = 0, stamp = 0; int32_t n = 0;
    int ok = fread(&magic, 4, 1, f) == 1 && fread(&stamp, 4, 1, f) == 1
          && fread(&n, 4, 1, f) == 1
          && magic == JD_CACHE_MAGIC && stamp == probe_stamp()
          && n == (int32_t)g_nr;
    if (ok) ok = (int)fread(g_st, sizeof(jd_stat), (size_t)n, f) == n;
    fclose(f);
    TR("PROBE cache %s (%s)\n", ok ? "hit" : "miss", p);
    return ok;
}

static void probe_cache_save(void)
{
    const char *p = probe_cache_path();
    if (!p) return;
    FILE *f = fopen(p, "wb");
    if (!f) return;
    uint32_t magic = JD_CACHE_MAGIC, stamp = probe_stamp();
    int32_t n = (int32_t)g_nr;
    fwrite(&magic, 4, 1, f); fwrite(&stamp, 4, 1, f); fwrite(&n, 4, 1, f);
    fwrite(g_st, sizeof(jd_stat), (size_t)n, f);
    fclose(f);
    TR("PROBE cache written: %s\n", p);
}

static void bags_init(void)
{
    for (int r = 0; r < R_NROLE; r++) {
        g_bag[r].n = 0; g_bag[r].head = 0xFFFF; g_bag[r].hpos = 0;
        g_bag[r].seed = mix32(0x51ED0000u + (uint32_t)r + g_run);
        for (int i = 0; i < JD_SEAM; i++) g_bag[r].hist[i] = 0xFFFF;
    }
    for (int i = 0; i < g_nr; i++) {
        jd_bag *b = &g_bag[g_st[i].role];
        if (b->n < JD_MAXR) b->items[b->n++] = (uint16_t)i;
    }
    for (int r = 0; r < R_NROLE; r++) g_bag[r].head = g_bag[r].n;  /* refill on 1st draw */
}

/* ============================================================
 * 3. palette: chained eased walk over a shuffled, de-duplicated bag
 * ============================================================ */

static void pal_features(void)
{
    for (int s = 0; s < g_ns; s++) {
        const uint32_t *p = jd_palette + (size_t)s * PAL_N;
        float hue[12] = {0}; double sat = 0, val = 0; int n = 0;
        for (int i = 0; i < PAL_N; i += 64) {
            uint32_t c = p[i];
            int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
            int mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
            int mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
            int d = mx - mn;
            val += mx / 255.0; sat += mx ? (double)d / mx : 0.0;
            if (d > 12) {
                float hh;
                if (mx == r)      hh = (float)(g - b) / d;
                else if (mx == g) hh = 2.0f + (float)(b - r) / d;
                else              hh = 4.0f + (float)(r - g) / d;
                if (hh < 0) hh += 6.0f;
                int bin = (int)(hh * 2.0f) % 12;
                hue[bin] += (float)d / 255.0f;
            }
            n++;
        }
        float tot = 0; for (int k = 0; k < 12; k++) tot += hue[k];
        if (tot < 1e-6f) tot = 1e-6f;
        for (int k = 0; k < 12; k++) g_pfeat[s][k] = hue[k] / tot;
        g_pfeat[s][12] = (float)(sat / n);
        g_pfeat[s][13] = (float)(val / n);
    }
    /* adjacency threshold = 55% of the median pairwise distance */
    float acc = 0; int m = 0;
    for (int i = 0; i < g_ns; i++)
        for (int j = i + 1; j < g_ns; j++) {
            float d = 0;
            for (int k = 0; k < 12; k++) {
                float x = g_pfeat[i][k] - g_pfeat[j][k]; d += x < 0 ? -x : x;
            }
            d = d * 0.5f;
            float ds = g_pfeat[i][12] - g_pfeat[j][12];
            float dv = g_pfeat[i][13] - g_pfeat[j][13];
            d += 1.4f * (ds < 0 ? -ds : ds) + 1.4f * (dv < 0 ? -dv : dv);
            acc += d; m++;
        }
    g_pthresh = m ? (acc / m) * 0.55f : 0.0f;
}

static float pal_dist(int a, int b)
{
    float d = 0;
    for (int k = 0; k < 12; k++) {
        float x = g_pfeat[a][k] - g_pfeat[b][k]; d += x < 0 ? -x : x;
    }
    d *= 0.5f;
    float ds = g_pfeat[a][12] - g_pfeat[b][12];
    float dv = g_pfeat[a][13] - g_pfeat[b][13];
    return d + 1.4f * (ds < 0 ? -ds : ds) + 1.4f * (dv < 0 ? -dv : dv);
}

static void pal_shuffle_raw(uint32_t epoch, uint8_t *out)
{
    for (int i = 0; i < g_ns; i++) out[i] = (uint8_t)i;
    uint32_t r = mix32((epoch ^ 0x5BF03635u) + g_run);
    for (int i = g_ns; i > 1; i--) {
        r = mix32(r);
        int j = (int)(r % (uint32_t)i);
        uint8_t t = out[i - 1]; out[i - 1] = out[j]; out[j] = t;
    }
}

/* Repair, not re-roll: walk once and swap forward past any scheme that is
 * too close to its predecessor.  The permutation survives, so every scheme
 * still plays exactly once per epoch — this only fixes ADJACENCY, which is
 * the half of "they all look the same" a uniform bag cannot touch. */
static void pal_build(uint32_t epoch, uint8_t *out, const uint8_t *prev_built)
{
    uint8_t prevbuf[256];
    const uint8_t *prev = prevbuf;
    pal_shuffle_raw(epoch, out);
    if (prev_built) prev = prev_built;          /* the real previous epoch */
    else            pal_shuffle_raw(epoch - 1, prevbuf);
    /* epoch seam: nothing in the first W of this epoch may appear in the
     * last W of the previous one, or a scheme can recur three legs apart
     * across the boundary */
    int W = g_ns / 4; if (W > 6) W = 6;
    for (int i = 0; i < W; i++) {
        int bad = 0;
        for (int j = g_ns - W; j < g_ns; j++) if (prev[j] == out[i]) { bad = 1; break; }
        if (!bad) continue;
        for (int j = W; j < g_ns; j++) {
            int clash = 0;
            for (int q = g_ns - W; q < g_ns; q++) if (prev[q] == out[j]) { clash = 1; break; }
            if (!clash) { uint8_t t = out[i]; out[i] = out[j]; out[j] = t; break; }
        }
    }
    int pc = prev[g_ns - 1];
    if (out[0] == pc && g_ns > 1) { uint8_t t = out[0]; out[0] = out[1]; out[1] = t; }
    for (int i = 0; i < g_ns; i++) {
        if (pal_dist(pc, out[i]) < g_pthresh) {
            for (int j = i + 1; j < g_ns; j++)
                if (pal_dist(pc, out[j]) >= g_pthresh) {
                    uint8_t t = out[i]; out[i] = out[j]; out[j] = t; break;
                }
        }
        pc = out[i];
    }
}

static int scheme_at(uint32_t leg)
{
    uint32_t epoch = leg / (uint32_t)g_ns, pos = leg % (uint32_t)g_ns;
    int slot = (int)(epoch & 1), other = slot ^ 1;
    if (g_pepoch[slot] != epoch) {
        const uint8_t *prev = (g_pepoch[other] == epoch - 1) ? g_pbag[other] : NULL;
        pal_build(epoch, g_pbag[slot], prev);
        g_pepoch[slot] = epoch;
    }
    return g_pbag[slot][pos];
}

static void layer_pal_build(int s);

static void palette_update(int frame)
{
    uint32_t leg = (uint32_t)frame >> 10;
    int A = scheme_at(leg), B = scheme_at(leg + 1);
    uint32_t x = ((uint32_t)frame & (JD_LEG - 1)) << 6;      /* 0..65472 */
    uint32_t t8 = ease_ss(x) >> 8;
    int key = (A << 17) | (B << 9) | (int)t8;
    if (key == g_blend_key) return;
    g_blend_key = key;
    const uint32_t *pa = jd_palette + (size_t)A * PAL_N;
    const uint32_t *pb = jd_palette + (size_t)B * PAL_N;
    uint32_t t = t8, it = 256 - t8;
    for (int i = 0; i < PAL_N; i++) {
        uint32_t ca = pa[i], cb = pb[i];
        uint32_t rb = (((ca & 0xFF00FFu) * it + (cb & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
        uint32_t g  = (((ca & 0x00FF00u) * it + (cb & 0x00FF00u) * t) >> 8) & 0x00FF00u;
        g_blend[i] = 0xFF000000u | rb | g;
    }
    /* Every live layer follows the shared ramp on the SAME frame.  Staggering
     * the rebuilds (round-robin) was measurably worse: a layer that misses a
     * few steps then catches up moves its whole image at once. */
    for (int s = 0; s < JD_NBUF; s++) if (g_L[s].live) layer_pal_build(s);
}

/* Build a layer's palette as a WINDOW into the shared ramp, then re-expand
 * it.  The window alone is what makes a layer read as a hue family instead
 * of a rainbow — but a narrow slice of a ramp is also a narrow slice of its
 * VALUE range, and that is grey mud, not "stark".  So the narrower the
 * window, the harder the value range is stretched back out to black..bright
 * and the more a dull slice gets its saturation restored.  Full-span layers
 * (the ground in a blazing mood) are left exactly as the palette author
 * wrote them. */
static void layer_pal_build(int s)
{
    uint32_t *d = g_pal[s], span = g_L[s].span, off = g_L[s].off;
    static int loop = -1;
    if (loop < 0) { const char *e = getenv("JD_PALLOOP"); loop = e ? atoi(e) : 1; }
    if (span >= PAL_N || !loop) {
        for (uint32_t i = 0; i < PAL_N; i++)
            d[i] = g_blend[(((i * span) >> 15) + off) & PAL_MASK];
    } else {
        const uint32_t body = PAL_N - PAL_RET;
        const uint32_t mul  = (span << 16) / body;
        const uint32_t top  = ((body - 1) * mul) >> 16;
        for (uint32_t i = 0; i < body; i++)
            d[i] = g_blend[(((i * mul) >> 16) + off) & PAL_MASK];
        for (uint32_t j = 0; j < PAL_RET; j++)
            d[body + j] = g_blend[(off + top - (top * (j + 1)) / PAL_RET) & PAL_MASK];
    }
    /* duplicate: patterns index pal[x & 0x7FFF], so handing them
     * (pal + k) stays in bounds for any k < PAL_N — that makes an
     * audio-driven colour rotation completely free at draw time. */

    static int noreshape = -1;
    if (noreshape < 0) noreshape = getenv("JD_NORESHAPE") ? 1 : 0;
    float k = noreshape ? 0.0f : 1.0f - (float)span / 24000.0f;
    if (k <= 0.02f) { memcpy(d + PAL_N, d, PAL_N * 4); return; }
    if (k > 0.85f) k = 0.85f;

    jd_layer *LL = &g_L[s];
    if (LL->lo_s < 0.0f) {
        /* Measure the window ONCE, at spawn, and freeze the transfer curve.
         * Re-measuring on every rebuild was itself a rough break: rebuilds
         * are 5-15 frames apart, so each re-smoothing step moved the whole
         * layer's colour at once.  A fixed monotone curve maps a smoothly
         * drifting ramp to a smoothly drifting image, by construction. */
        int lo = 255, hi = 0; long ssum = 0; int n = 0;
        for (int i = 0; i < PAL_N; i += 64) {
            uint32_t c = d[i];
            int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
            int v = r > g ? (r > b ? r : b) : (g > b ? g : b);
            int m = r < g ? (r < b ? r : b) : (g < b ? g : b);
            if (v < lo) lo = v; if (v > hi) hi = v; ssum += v - m; n++;
        }
        /* cap the contrast gain at 2x: stretching a narrow window to full
         * range also multiplies the routine's frame-to-frame motion */
        if (hi - lo < 123) hi = lo + 123;
        float ms = (float)ssum / (n ? n : 1) / 255.0f;
        float gs = ms > 0.02f ? 0.38f / ms : 1.0f;
        if (gs < 1.0f) gs = 1.0f;
        if (gs > 1.5f) gs = 1.5f;
        gs = 1.0f + (gs - 1.0f) * k;
        LL->lo_s = (float)lo; LL->hi_s = (float)hi; LL->gs_s = gs;
        LL->gq = (uint32_t)(gs * 256.0f);
        for (int v = 0; v < 256; v++) {
            float t = (float)(v - lo) / (float)(hi - lo);
            if (t < 0) t = 0; if (t > 1) t = 1;
            float o = (float)v + (6.0f + t * 245.0f - (float)v) * k;
            LL->lut[v] = (uint8_t)(o < 0 ? 0 : o > 255 ? 255 : o);
        }
        TR("PAL s=%d span=%u lo=%d hi=%d gs=%.2f k=%.2f\n", s, span, lo, hi, gs, k);
    }
    const uint8_t *lut = LL->lut;
    uint32_t gq = LL->gq;
    /* reciprocal table: the reshape runs 32768 entries per layer per palette
     * step, and an integer divide in there is the whole cost */
    static uint16_t recip[256];
    if (!recip[1]) for (int i = 1; i < 256; i++) recip[i] = (uint16_t)(65535 / i);

    for (int i = 0; i < PAL_N; i++) {
        uint32_t c = d[i];
        uint32_t r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
        uint32_t v = r > g ? (r > b ? r : b) : (g > b ? g : b);
        uint32_t m = r < g ? (r < b ? r : b) : (g < b ? g : b);
        uint32_t sp = v - m, nv = lut[v];
        if (!sp) { d[i] = 0xFF000000u | (nv << 16) | (nv << 8) | nv; continue; }
        uint32_t ns = (sp * gq) >> 8; if (ns > nv) ns = nv;
        uint32_t inv = ns * recip[sp], base = nv - ns;   /* Q16 */
        uint32_t nr = base + (((r - m) * inv) >> 16);
        uint32_t ng = base + (((g - m) * inv) >> 16);
        uint32_t nb = base + (((b - m) * inv) >> 16);
        d[i] = 0xFF000000u | (nr > 255 ? 255u : nr) << 16
                           | (ng > 255 ? 255u : ng) << 8
                           | (nb > 255 ? 255u : nb);
    }
    /* duplicate AFTER the reshape (review 03 F1): patterns index
     * pal[x & 0x7FFF], so handing them (pal + k) stays in bounds for any
     * k < PAL_N and the audio rotation can slide freely.  Copying before the
     * reshape left the upper half un-reshaped, so a rotation reaching into it
     * changed the colour transfer mid-frame. */
    memcpy(d + PAL_N, d, PAL_N * 4);
}

/* ============================================================
 * 4. blend kernels
 * ============================================================ */

static void span_scale(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    if (w >= 256) { memcpy(dst, src, (size_t)n * 4); return; }
    for (int i = 0; i < n; i++) {
        uint32_t c = src[i];
        uint32_t rb = (((c & 0xFF00FFu) * w) >> 8) & 0xFF00FFu;
        uint32_t g  = (((c & 0x00FF00u) * w) >> 8) & 0x00FF00u;
        dst[i] = 0xFF000000u | rb | g;
    }
}

/* Saturating brightness gain (w > 256 lifts).  Used only by the dead-air
 * guard: a screensaver must never sit on a near-black screen while an
 * accumulator draws its first strokes.  Per-channel clamp keeps hues honest. */
static void span_gain(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    if (w <= 256) { span_scale(dst, src, n, w); return; }
    for (int i = 0; i < n; i++) {
        uint32_t c = src[i];
        uint32_t r = (((c >> 16) & 255) * w) >> 8;
        uint32_t g = (((c >>  8) & 255) * w) >> 8;
        uint32_t b = ( (c        & 255) * w) >> 8;
        if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
        dst[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }
}

static void span_lerp(uint32_t *dst, const uint32_t *a, const uint32_t *b,
                      int n, uint32_t wb)
{
    uint32_t wa = 256 - wb;
    for (int i = 0; i < n; i++) {
        uint32_t ca = a[i], cb = b[i];
        uint32_t rb = (((ca & 0xFF00FFu) * wa + (cb & 0xFF00FFu) * wb) >> 8) & 0xFF00FFu;
        uint32_t g  = (((ca & 0x00FF00u) * wa + (cb & 0x00FF00u) * wb) >> 8) & 0x00FF00u;
        dst[i] = 0xFF000000u | rb | g;
    }
}

/* MAX: wherever the overlay is darker than the ground, the ground survives
 * bit-exact.  That is what keeps a stack readable instead of grey. */
static void span_max(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        uint32_t srb = (((s & 0x00FF00FFu) * w) >> 8) & 0x00FF00FFu;
        uint32_t sg  = (((s & 0x0000FF00u) * w) >> 8) & 0x0000FF00u;
        uint32_t dr = d & 0x00FF0000u, dg = d & 0x0000FF00u, db = d & 0xFFu;
        uint32_t sr = srb & 0x00FF0000u, sb = srb & 0xFFu;
        dst[i] = 0xFF000000u | (dr > sr ? dr : sr) | (dg > sg ? dg : sg)
                             | (db > sb ? db : sb);
    }
}

static void span_screen(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        uint32_t dr = (d >> 16) & 255, dg = (d >> 8) & 255, db = d & 255;
        uint32_t sr = (((s >> 16) & 255) * w) >> 8;
        uint32_t sg = (((s >>  8) & 255) * w) >> 8;
        uint32_t sb = (( s        & 255) * w) >> 8;
        uint32_t r = dr + sr - ((dr * sr) >> 8);
        uint32_t g = dg + sg - ((dg * sg) >> 8);
        uint32_t b = db + sb - ((db * sb) >> 8);
        dst[i] = 0xFF000000u | (r > 255 ? 255u : r) << 16
                             | (g > 255 ? 255u : g) << 8
                             | (b > 255 ? 255u : b);
    }
}

static void span_add(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        uint32_t r = ((d >> 16) & 255) + ((((s >> 16) & 255) * w) >> 8);
        uint32_t g = ((d >>  8) & 255) + ((((s >>  8) & 255) * w) >> 8);
        uint32_t b = ( d        & 255) + (((  s        & 255) * w) >> 8);
        dst[i] = 0xFF000000u | (r > 255 ? 255u : r) << 16
                             | (g > 255 ? 255u : g) << 8
                             | (b > 255 ? 255u : b);
    }
}

/* difference, weight-limited — the stark end of the range without the strobe */
static void span_diff(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    uint32_t iw = 256 - w;
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        uint32_t dr = (d >> 16) & 255, dg = (d >> 8) & 255, db = d & 255;
        uint32_t sr = (s >> 16) & 255, sg = (s >> 8) & 255, sb = s & 255;
        uint32_t xr = dr > sr ? dr - sr : sr - dr;
        uint32_t xg = dg > sg ? dg - sg : sg - dg;
        uint32_t xb = db > sb ? db - sb : sb - db;
        dst[i] = 0xFF000000u | (((dr * iw + xr * w) >> 8) << 16)
                             | (((dg * iw + xg * w) >> 8) <<  8)
                             |  ((db * iw + xb * w) >> 8);
    }
}

static void blend_span(uint32_t *dst, const uint32_t *src, int n,
                       uint32_t w, int mode)
{
    if (!w) return;
    switch (mode) {
        case B_MAX:    span_max(dst, src, n, w);    break;
        case B_SCREEN: span_screen(dst, src, n, w); break;
        case B_ADD:    span_add(dst, src, n, w);    break;
        case B_DIFF:   span_diff(dst, src, n, w);   break;
        default:       span_lerp(dst, dst, src, n, w); break;
    }
}

/* ============================================================
 * 5. scheduling
 * ============================================================ */

/* RHYTHM (review 05): the old 4-entry LIFE_S table collapsed to two values
 * per slot once the JD_MAXSL cap was applied (ground 1380|1640, mid 1380|1610),
 * so every ground handover was 24.0 or 28.3 s and every mid tenancy 40 s —
 * a metronome.  Hold is now drawn CONTINUOUSLY in [HOLD_LO, HOLD_HI] and
 * scaled by the tempo of the ground it plays under (g_tempo). */
static const int HOLD_LO[JD_NSLOT] = { 1080,  900,  700,  500 };
static const int HOLD_HI[JD_NSLOT] = { 1640, 1610, 1500, 1200 };
static const int FADE_IN[JD_NSLOT]  = { 180, 180, 150, 120 };
static const int FADE_OUT[JD_NSLOT] = { 180, 210, 180, 150 };
static const int REST_LO[JD_NSLOT]  = {   0, 180, 300, 480 };
static const int REST_HI[JD_NSLOT]  = {   0, 480, 720, 1080 };
/* peak weight per slot, Q8 — an overlay that reaches 256 has replaced the
 * ground instead of enriching it */
static const uint16_t PEAK_LO[JD_NSLOT] = { 256, 128, 100,  72 };
static const uint16_t PEAK_HI[JD_NSLOT] = { 256, 180, 140, 115 };

/* palette window span per slot per mood: stark = a narrow slice of the ramp
 * (near-monochrome), blaze = the whole wheel */
static const uint32_t SPAN[M_N][JD_NSLOT] = {
    { 14000, 8000, 4500, 2600 },      /* M_STARK */
    { 20000, 14000, 8000, 4000 },     /* M_RICH  */
    { 32768, 22000, 12000, 6000 },    /* M_BLAZE */
};


static int g_gpostponed = 0;              /* frames the ground's exit is deferred */
static int g_audition = 0;
static uint32_t g_gluma = 60;               /* ground buffer luma EWMA */
static int cand_slot;                     /* slot the admission test is for */
static int g_force = 0;                   /* emergency: correctness only    */
static int g_now = 0;                     /* current frame, for cadence     */
static int g_exclude = -1;                /* slot being replaced this spawn */

/* frames of envelope an asm layer needs inside one draw.s period */
static int asm_room(int rt, int frame, int sidx)
{
    int room = 1024 - (frame & 1023) - 30;          /* its palette walk steps */
    if (rt >= JD_ASM_ACC0) {                        /* and its canvas clears  */
        int r2 = 2048 - (frame & 2047) - 90;
        if (r2 < room) room = r2;
    }
    return room - FADE_IN[sidx] - FADE_OUT[sidx];   /* hold it can afford */
}

/* Q8 motion amplification a slot's palette window will apply (see
 * layer_pal_build: the narrower the window, the harder it is stretched). */
static uint32_t amp_q8(int sidx)
{
    float k = 1.0f - (float)SPAN[g_mood][sidx] / 24000.0f;
    if (k < 0.0f) k = 0.0f;
    if (k > 0.85f) k = 0.85f;
    return (uint32_t)(256.0f * (1.0f + k));
}

static int admissible(uint16_t r, int slot)
{
    const jd_stat *st = &g_st[r];
    int slot_i0 = (slot == JD_SHADOW) ? 0 : slot;
    /* 1. a routine may never be live twice — patterns hold file-static state */
    for (int i = 0; i < JD_NBUF; i++)
        if (g_L[i].live && g_L[i].routine == (int)r) return 0;
    /* 2. asm modes are grounds only (they take no layer palette) */
    if (r < JD_NASM && slot != 0 && slot != JD_SHADOW) return 0;
    /* 3. an asm mode must fit its whole envelope inside one draw.s period
     *    (see try_spawn) — check it HERE so the bag defers rather than
     *    burning the draw.  This is FEASIBILITY, not taste, so it binds
     *    under g_force too: try_spawn rejects the pick a second time on
     *    exactly this test and returns without trying another bag, so
     *    letting the emergency path hand back an asm mode that cannot fit
     *    means no ground at all.  Measured before this moved above the
     *    g_force line: at launch the only routines with a GROUND role are
     *    the 24 asm modes (patterns are unprobed and default to FIGURE),
     *    every one of them was refused on room, and the screen stayed
     *    BLACK for 150 frames — then cut to a full picture in one frame,
     *    delta 160.6.  Below the line, the emergency path falls through to
     *    the pattern bag and the ground is up on frame one. */
    if (r < JD_NASM && asm_room(r, g_now, slot_i0) < 120) return 0;
    /* the three rules above are correctness; everything below is taste and
     * budget, and may be waived when the alternative is a broken frame */
    if (g_force) return 1;
    /* 4. while the library is still being measured, only play what has
     *    actually been measured — an unprobed routine has guessed motion */
    if (!st->probed && !g_probe_done) return 0;
    /* 4. budget: live renders + this one + a blend allowance.  The tenant
     *    being replaced does not count — it is on its way out. */
    int cost = st->cost_q8, n = 1;
    for (int i = 0; i < JD_NBUF; i++)
        if (g_L[i].live && i != g_exclude) { cost += g_st[g_L[i].routine].cost_q8; n++; }
    cost += n * 120;                       /* ~0.47 ms per blend, Q8 */
    if (cost > BUDGET_Q8) return 0;
    /* 5. motion: weight each layer's measured delta by its peak opacity.
     *    A narrow palette window is stretched back to full contrast, and
     *    that multiplies the ROUTINE's motion too — so a candidate for a
     *    narrow slot is charged for the amplification it will get. */
    int slot_i = slot_i0;
    uint32_t d = ((uint32_t)st->delta_q8 * PEAK_HI[slot_i]) >> 8;
    d = (d * amp_q8(slot_i)) >> 8;
    for (int i = 0; i < JD_NBUF; i++)
        if (g_L[i].live && i != g_exclude)
            d += ((uint32_t)g_st[g_L[i].routine].delta_q8 * g_L[i].w_peak) >> 8;
    if (d > (uint32_t)DCAP_Q8) return 0;
    /* until the library is fully measured, keep the opening calm */
    if (!g_probe_done && d > (uint32_t)(DCAP_Q8 / 2)) return 0;
    /* 6. an overlay must have somewhere to be seen through: it has to be at
     *    least a third near-black, or it just paints over the ground */
    if (slot_i != 0 && st->dark < 70) return 0;
    return 1;
}

static int admissible_cb(uint16_t r, int slot) { (void)slot; return admissible(r, cand_slot); }

/* the two roles each slot may host — nothing else is ever admitted there */
static const uint8_t SLOT_ROLE[JD_NSLOT][2] = {
    { R_GROUND, R_FIELD }, { R_FIELD, R_FIGURE },
    { R_FIGURE, R_SPARK }, { R_SPARK,  R_FIGURE },
};

static int pick_role(int slot, uint32_t r)
{
    static const uint8_t tab[JD_NSLOT][4] = {
        { R_GROUND, R_GROUND, R_GROUND, R_FIELD  },
        { R_FIELD,  R_FIELD,  R_FIGURE, R_FIGURE },
        { R_FIGURE, R_FIGURE, R_SPARK,  R_SPARK  },
        { R_SPARK,  R_SPARK,  R_SPARK,  R_FIGURE },
    };
    int want = tab[slot][r & 3];
    if (g_bag[want].n) return want;
    for (int j = 0; j < 2; j++) if (g_bag[SLOT_ROLE[slot][j]].n) return SLOT_ROLE[slot][j];
    for (int k = 0; k < R_NROLE; k++) if (g_bag[k].n) return k;
    return R_GROUND;
}

static int pick_blend(const jd_stat *st, uint32_t r, uint16_t *peak, int slot)
{
    uint32_t lo = PEAK_LO[slot], hi = PEAK_HI[slot];
    uint32_t p = lo + (r >> 16) % (hi - lo + 1);
    int mode = B_MAX;
    if (st->dark >= 215 && st->sat >= 25) mode = B_SCREEN;
    if (g_mood == M_BLAZE && st->dark >= 180) mode = B_SCREEN;
    if (st->delta_q8 < 512 && (r & 15) == 3) {          /* seasoning */
        mode = B_DIFF;
        if (p > 90) p = 90;
    }
    if (mode == B_SCREEN) {
        /* cap by the ground's brightness — screen over a dark ground goes
         * milky fast (compositor.md §3.3) */
        uint32_t gl = g_L[0].live ? g_st[g_L[0].routine].luma : 100;
        uint32_t cap = gl >= 160 ? 154u : gl <= 40 ? 64u : 64u + (gl - 40) * 90u / 120u;
        if (p > cap) p = cap;
    }
    *peak = (uint16_t)p;
    return mode;
}

/* Give an accumulator the canvas it was WRITTEN for.
 *
 * draw.s modes 15..23 own their buffer: on the frame where (frame & 2047)
 * is zero they fill it with an entry background and from then on they only
 * ADD to it.  Those backgrounds are not black — modes 17 and 20 paint
 * WHITE (0xF2EEF2 / 0xE8F0F6) because they draw dark slashes and frost on
 * a light field; 23 paints deep magenta; the rest paint a tinted ink.
 *
 * But asm_room deliberately fits a layer's whole envelope INSIDE one 2048
 * period, so that entry frame never arrives during a tenancy, and the
 * engine was substituting `memset(buf, 0)` — pure black.  Modes 17 and 20
 * then drew dark-on-dark and put up a screen that measured luma 0.0 for
 * its entire life; the others lost their tint and opened near-black.  This
 * was the largest single source of dead frames in the battery.
 *
 * So call the routine at its own period boundary instead: it lays down its
 * real canvas, and then a few hundred more calls accumulate the strokes it
 * would have drawn in the first seconds of a segment (they are hashed per
 * frame, so starting the sequence early is invisible).  Bounded by wall
 * clock, because this runs inside a frame. */
static void canvas_prime(jd_layer *L, int rt, int frame)
{
    int base = frame & ~2047;
    g_mode = (uint32_t)rt;
    double t0 = now_ms();
    for (int i = 0; i < 512; i++) {
        draw_frame(L->buf, g_w, g_h, base + i);
        if ((i & 15) == 15 && now_ms() - t0 > 5.0) break;
    }
    TR("PRIME rt=%d %.1f ms\n", rt, now_ms() - t0);
}

static int try_spawn(int slot, int frame)
{
    int sidx = (slot == JD_SHADOW) ? 0 : slot;
    g_now = frame;
    /* the tenant this spawn replaces is leaving, so it must not veto the
     * replacement on budget or motion grounds */
    g_exclude = (slot == JD_SHADOW && g_L[0].live && frame >= g_L[0].t_out - 180) ? 0 : -1;
    uint32_t r = mix32((uint32_t)frame * 2654435761u + (uint32_t)slot * 7919u);
    cand_slot = slot;
    int role = pick_role(sidx, r);
    uint16_t v = bag_draw(&g_bag[role], admissible_cb, slot);
    if (v == 0xFFFF) {
        /* try the other roles this slot may host before giving up */
        /* a slot may only ever host its own two roles — a FIELD routine in
         * the spark slot paints over everything under it */
        for (int j = 0; j < 2 && v == 0xFFFF; j++) {
            int k = SLOT_ROLE[sidx][j];
            if (k != role && g_bag[k].n) v = bag_draw(&g_bag[k], admissible_cb, slot);
        }
        /* The screen may never be empty, and an asm incumbent may never be
         * carried across its own draw.s period boundary.  In both cases the
         * ONLY acceptable answer is a successor, so scan every bag.
         *
         * The second case is why `g_force` is in the test.  Slot 0 hosts
         * R_GROUND and R_FIELD only, and until the probe finishes R_GROUND
         * holds nothing but the 24 asm modes while R_FIELD is empty and all
         * 200 patterns sit in R_FIGURE.  Near a 1024-frame boundary every
         * asm mode fails asm_room, so the caller's forced retry found
         * nothing, the incumbent's exit was postponed instead, and it was
         * still on screen when draw.s stepped its palette walk — a
         * full-frame cut at 100% weight, measured 43.3 / 50.8 / 62.0 /
         * 108.9 on four of the thirty sampled starts.  Under g_force a
         * FIGURE routine on the ground is a far smaller price. */
        if (v == 0xFFFF && sidx == 0 &&
            (g_force || (!g_L[0].live && !g_L[JD_SHADOW].live))) {
            int had = g_force;
            g_force = 1;
            for (int k = 0; k < R_NROLE && v == 0xFFFF; k++)
                if (g_bag[k].n) v = bag_draw(&g_bag[k], admissible_cb, slot);
            g_force = had;
        }
        if (v == 0xFFFF) { TR("NOSPAWN f=%d slot=%d role=%d bags=%u/%u/%u/%u\n",
                              frame, slot, role, g_bag[0].n, g_bag[1].n, g_bag[2].n, g_bag[3].n);
                           return 0; }
    }

    jd_layer *L = &g_L[slot];
    const jd_stat *st = &g_st[v];
    L->routine = v;
    L->seed    = mix32(r ^ 0xA5A5A5A5u);
    /* Patterns animate off `frame`, in float.  main.c starts the counter at
     * a random value up to 4.2M, and at that magnitude a per-frame phase
     * step of a few hundredths of a radian quantises away: measured, one
     * pattern's frame-to-frame delta goes from 0.34 at frame 3 000 to 19.1
     * at frame 100 000 — the engine's single largest source of jitter, and
     * it has been there since v2.0.  Give every layer its own small clock. */
    L->fbase   = 300 + (int)(L->seed % 2500u);
    L->live    = 1;
    L->sl      = 0;
    L->cls     = st->cls;
    L->cost_ms = st->cost_q8 / 256.0;
    L->half    = 0;
    L->frozen  = 0;
    L->strikes = 0;
    L->parity  = (uint8_t)(slot & 1);

    if (sidx == 0) {
        L->blend  = B_MIX;
        L->w_peak = 256;
        /* a new ground picks the mood for everything spawned under it */
        int m = (int)((r >> 8) % M_N);
        if (m == g_prev_mood && m == M_STARK) m = M_RICH;
        g_prev_mood = g_mood; g_mood = m;
        /* and the PACE of everything spawned under it: a slow ground hosts a
         * sparse, lingering stack; a quick one a busy stack.  Drifts the
         * whole show's period over minutes instead of holding one beat. */
        g_tempo = 180u + mix32(r ^ 0x7E3D0u) % 161u;             /* 0.70..1.33 */
        g_lead  = 90 + (int)(mix32(r ^ 0x1EADu) % 91u);           /* 90..180   */
    } else {
        uint16_t pk; L->blend = (uint8_t)pick_blend(st, r, &pk, sidx); L->w_peak = pk;
    }

    g_exclude = -1;
    int fin = FADE_IN[sidx], fout = FADE_OUT[sidx];
    uint32_t hr = mix32(r ^ 0xB0B0B0B0u);
    int hold = HOLD_LO[sidx] + (int)(hr % (uint32_t)(HOLD_HI[sidx] - HOLD_LO[sidx] + 1));
    if (sidx) hold = (int)(((uint32_t)hold * g_tempo) >> 8);   /* overlays follow the ground's tempo */
    if (v >= JD_NASM) {
        /* envelope jitter (review 01 F9 / 05): 0.6x..1.6x per spawn, and one
         * overlay in 16 drifts in over twice the time.  asm modes keep the
         * table values — asm_room budgets on them. */
        uint32_t j = mix32(r ^ 0x51DE5EEDu);
        fin  = FADE_IN[sidx]  * (154 + (int)(j & 255)) >> 8;
        fout = FADE_OUT[sidx] * (154 + (int)((j >> 8) & 255)) >> 8;
        if (sidx && ((j >> 16) & 15) == 0) fin *= 2;
    }
    if (v < JD_NASM) {
        /* Two hard cadences belong to draw.s, not to this scheduler:
         *   - every 1024 frames it steps its OWN palette walk, and that step
         *     is a full-frame cut (measured 55 mean channel delta);
         *   - modes 15..23 own their canvas and self-clear every 2048.
         * Neither can be crossfaded from here, so a layer holding an asm
         * mode must live and die entirely inside one period. */
        int room = asm_room(v, frame, sidx);
        if (room < 120) { L->live = 0; return 0; }
        if (hold > room) hold = room;
        if (st->cls == C_CANVAS) canvas_prime(L, v, frame);
    }
    if (fin + hold + fout > JD_MAXSL) hold = JD_MAXSL - fin - fout;

    L->t_in = frame; L->t_full = frame + fin;
    L->t_out = L->t_full + hold; L->t_end = L->t_out + fout;

    g_lsig_ok[slot] = 0;
    L->mdel = 0;
    L->lo_s = -1.0f;
    L->span = SPAN[g_mood][sidx];
    if (L->span > PAL_N) L->span = PAL_N;
    L->off  = L->seed & PAL_MASK;
    layer_pal_build(slot);
    g_last_change = frame;
    g_gap = 30 + (int)(mix32(r ^ 0x6A9F00Du) % 211u);       /* 0.5..4 s, never the same twice */
    TR("SPAWN f=%d slot=%d rt=%d role=%d blend=%d peak=%d life=%d mood=%d span=%u\n",
       frame, slot, (int)v, g_st[v].role, L->blend, L->w_peak, hold, g_mood, L->span);
    return 1;
}

static uint32_t layer_w_q16(const jd_layer *L, int f)
{
    if (f <= L->t_in || f >= L->t_end) return 0;
    uint32_t e;
    if (f >= L->t_full && f <= L->t_out) e = 65536;
    else if (f < L->t_full)
        e = ease_ss((uint32_t)(((int64_t)(f - L->t_in) << 16) / (L->t_full - L->t_in)));
    else
        e = ease_ss((uint32_t)(((int64_t)(L->t_end - f) << 16) / (L->t_end - L->t_out)));
    return (e * L->w_peak) >> 8;          /* Q16: peak is Q8, e is Q16 */
}

static void sched_tick(int frame)
{
    /* retire */
    for (int s = 0; s < JD_NBUF; s++) {
        jd_layer *L = &g_L[s];
        if (!L->live || frame < L->t_end) continue;
        /* the ground never leaves without a successor in place, or the frame
         * would go black */
        if (s == 0 && !g_L[JD_SHADOW].live) { L->t_end = frame + 60; continue; }
        L->live = 0;
        if (s == JD_SHADOW) continue;
        {
            int lo = (int)(((uint32_t)REST_LO[s] * g_tempo) >> 8);
            int hi = (int)(((uint32_t)REST_HI[s] * g_tempo) >> 8);
            uint32_t rr = mix32((uint32_t)frame ^ ((uint32_t)s << 20) ^ g_run);
            int rest = lo + (int)(rr % (uint32_t)(hi - lo + 1));
            uint32_t k = (rr >> 24) & 15u;
            if (k < 2)       rest = rest * 5 / 2;   /* 1 in 8: a real gap, the stack thins */
            else if (k == 2) rest = lo / 2;         /* 1 in 16: straight back — a double  */
            g_rest[s] = frame + rest;
        }
    }
    /* base handover: promote the successor once the incumbent has gone */
    if (!g_L[0].live && g_L[JD_SHADOW].live) {
        jd_layer t = g_L[0];
        g_L[0] = g_L[JD_SHADOW];
        g_L[JD_SHADOW] = t;
        uint32_t *pb = g_pal[0]; g_pal[0] = g_pal[JD_SHADOW]; g_pal[JD_SHADOW] = pb;
        uint32_t *bb = g_buf[0]; g_buf[0] = g_buf[JD_SHADOW]; g_buf[JD_SHADOW] = bb;
        g_L[0].buf = g_buf[0]; g_L[0].pal = g_pal[0];
        /* The motion signature is per SLOT, not per layer, so it has to
         * travel with the tenant.  Leaving it behind makes the promoted
         * ground's first measured step a comparison against the OUTGOING
         * ground's pixels — a whole-frame difference, which the strobe
         * detector below reads as a JUMP and answers by freezing the fresh
         * ground and bringing its handover forward.  That self-perpetuates:
         * measured before this line existed, every ground was frozen at
         * sl==235 and replaced 240 frames later, for ever. */
        { uint32_t tmp[512];
          memcpy(tmp,               g_lsig[0],         sizeof tmp);
          memcpy(g_lsig[0],         g_lsig[JD_SHADOW], sizeof tmp);
          memcpy(g_lsig[JD_SHADOW], tmp,               sizeof tmp); }
        { uint8_t t2 = g_lsig_ok[0];
          g_lsig_ok[0] = g_lsig_ok[JD_SHADOW]; g_lsig_ok[JD_SHADOW] = t2; }
        g_L[JD_SHADOW].live = 0;
        g_gpostponed = 0; g_audition = 0;
        /* review 05: overlays whose rest expired DURING the handover would all
         * release on this exact frame (measured: 25% of overlay arrivals landed
         * 0-2 s after a promotion, 11% on the promotion frame itself), which
         * reads as a rhythm.  Spread them across the next ~9 s. */
        for (int s = 1; s < JD_NSLOT; s++)
            if (!g_L[s].live && g_rest[s] < frame + 60)
                g_rest[s] = frame + (int)(mix32((uint32_t)frame * 31u
                                                + (uint32_t)s + g_run) % 541u);
    }
    int handover = g_L[JD_SHADOW].live;

    /* base: spawn at start, or start the successor before the incumbent dies */
    if (!g_L[0].live && !handover) {
        g_gpostponed = 0;
        if (frame >= g_rest[0] && !try_spawn(0, frame)) g_rest[0] = frame + 30;
    }
    else if (!handover && g_L[0].live && frame >= g_L[0].t_out - g_lead) {
        /* Start the successor 2 s BEFORE the incumbent begins to fade; if
         * nothing is admissible yet, postpone the incumbent's exit rather
         * than leave the ground fading alone (a dimming frame).
         * BUT a canvas-owning asm ground self-clears on the global 2048
         * cadence — postponing one past that point blanks the picture at
         * full weight, which is the worst break in the engine.  So near
         * that edge, or after 5 s of refusals, force a successor. */
        if (try_spawn(JD_SHADOW, frame)) { handover = 1; g_gpostponed = 0; }
        else {
            int clear_soon = g_L[0].routine < JD_NASM &&
                             (1024 - (frame & 1023)) < FADE_OUT[0] + 120;
            /* `frame >= t_out + 300` can never fire: the postponement below
             * pushes t_out 30 frames per 30 frames elapsed, so the deadline
             * runs away from the clock at exactly the speed of the clock.
             * Measured: 3727 consecutive frames of refusals with the ground
             * pinned live.  Count the deferral instead of chasing t_out. */
            if (clear_soon || g_gpostponed >= 300) {
                g_force = 1;
                if (try_spawn(JD_SHADOW, frame)) handover = 1;
                g_force = 0;
                g_gpostponed = 0;
            } else if (frame >= g_L[0].t_out) {
                g_L[0].t_out += 30; g_L[0].t_end += 30; g_gpostponed += 30;
            }
        }
    }

    /* overlays */
    for (int s = 1; s < JD_NSLOT; s++) {
        if (s >= g_slot_cap) { continue; }
        jd_layer *L = &g_L[s];
        /* AUDIO: a downbeat pulls the next layer forward (never later), so
         * new material arrives ON the music instead of on a blind clock.
         * beat > 700 = within two frames of the onset; a 2.5 s window catches
         * most entries without ever making a layer arrive early twice. */
        if (!L->live && g_audio.live && g_audio.beat > 700
            && frame < g_rest[s] && g_rest[s] - frame < 150)
            g_rest[s] = frame;
        if (L->live || frame < g_rest[s]) continue;
        /* never two layers arriving at once: 1 s minimum between entries,
         * and nothing enters while the ground is being handed over */
        /* under load, thin the stack to base+mid — but never to a bare
         * ground: a solo layer has nothing to dilute its motion */
        if (handover || (g_hot && s >= 2) || frame - g_last_change < g_gap) continue;
        if (!try_spawn(s, frame)) g_rest[s] = frame + 120;
    }
}

/* ============================================================
 * 6. frame
 * ============================================================ */

static void engine_init(uint32_t *fb, int w, int h, int frame)
{
    (void)fb;
    g_w = w; g_h = h; g_frame0 = frame;
    g_run = mix32((uint32_t)frame * 2654435761u + 0x1D0F1E55u); g_gpostponed = 0;
    g_nr = JD_NASM + jd_pattern_count;
    if (g_nr > JD_MAXR) g_nr = JD_MAXR;
    g_ns = JD_NS;
    { const char *e = getenv("JD_NS"); if (e) { int v = atoi(e); if (v > 0 && v < 256) g_ns = v; } }
    { const char *e = getenv("JD_LAYERS");
      if (e) { int v = atoi(e); if (v >= 1 && v <= JD_NSLOT) g_slot_cap = v; } }

    for (int i = 0; i < JD_NBUF; i++) {
        g_buf[i] = (uint32_t*)calloc((size_t)w * h, 4);
        g_pal[i] = (uint32_t*)calloc(PAL_N * 2, 4);   /* 2x: audio rotation window */
        g_L[i].buf = g_buf[i]; g_L[i].pal = g_pal[i];
        g_L[i].live = 0; g_rest[i] = frame;
    }
#if JD_TRACE
    { const char *e = getenv("JD_DEBUG"); g_dbg = e ? atoi(e) : 0; if (e && !g_dbg) g_dbg = 1; }
#endif
    pal_features();
    stats_defaults();
    if (probe_cache_load()) {
        g_probe_done = 1;         /* every routine already sorted into a layer */
    } else {
        probe_begin(w * h);
        probe_step(250.0);        /* enough to start with real material */
    }
    bags_init();
    TR("INIT routines=%d schemes=%d bags=%u/%u/%u/%u palthresh=%.3f\n", g_nr, g_ns,
       g_bag[0].n, g_bag[1].n, g_bag[2].n, g_bag[3].n, g_pthresh);

    /* J's cascade: base now, mid +3 s, accent +4 s, spark +9 s */
    /* ... jittered per launch, so no two openings have the same beat */
    g_rest[1] = frame + 120 + (int)(mix32(g_run ^ 1u) % 241u);   /* 2..6 s  */
    g_rest[2] = frame + 240 + (int)(mix32(g_run ^ 2u) % 421u);   /* 4..11 s */
    g_rest[3] = frame + 480 + (int)(mix32(g_run ^ 3u) % 601u);   /* 8..18 s */
    g_ready = 1;
}

/* ---- AUDIO -> COLOUR.  Level and treble keep the whole ramp sliding, and
 * every beat kicks it a further step; the offset only ever moves forward,
 * through an eased velocity, so hues TRAVEL with the music instead of
 * jumping.  Free at draw time because every layer palette is stored twice
 * (see layer_pal_build).
 * Speed, measured against real tracks with the tap (level ~600, treble
 * ~400, beat avg ~300): ~40 palette steps/frame = one full turn of the ramp
 * in ~14 s; a loud, bright peak (level 900, treble 900, beat 1024) reaches
 * ~125/frame = 4.4 s per turn.  In silence the velocity decays and the
 * colours settle exactly where they are. */
static void audio_rotate(void)
{
    static uint32_t rot, vel;
    if (g_audio.live) {
        uint32_t target = ((uint32_t)g_audio.level  * 2
                         + (uint32_t)g_audio.treble * 3
                         + (uint32_t)g_audio.beat   * 8) >> 4;
        vel = vel + ((target > vel ? target - vel : 0) >> 3)   /* attack ~8 f */
                  - ((vel > target ? vel - target : 0) >> 4);  /* release ~16 */
    } else {
        vel -= vel >> 5;
    }
    rot += vel >> 3;
    g_prot = rot & (PAL_N - 1);
}

/* ---- AUDIO -> WEIGHT.  Overlays surge with the low end: 0.45x in a
 * breakdown, 1.0x at a typical track's median bass (~470/1024 through the
 * tap), up to 1.6x on the drop.  The multiplier itself is eased (attack
 * ~4 frames, release ~16) so a kick is a push, not a flash. */
static uint32_t g_surge = 256;
static void audio_surge(void)
{
    uint32_t k = g_audio.live ? 115 + ((uint32_t)g_audio.bass * 295 >> 10) : 256;
    if (k > g_surge) g_surge += (k - g_surge) >> 2;
    else             g_surge -= (g_surge - k) >> 4;
}

/* JD_MODE=N — one routine, no layers, v2.0 semantics for per-pattern tests */
static int mode_override(uint32_t *fb, int w, int h, int frame)
{
    static int ov = -2;
    if (ov == -2) { const char *e = getenv("JD_MODE"); ov = e ? atoi(e) : -1; }
    if (ov < 0) return 0;
    palette_update(frame);

    audio_rotate();                 /* AUDIO -> COLOUR */
    int m;
    if (ov >= 1 && ov <= jd_pattern_count) m = JD_NASM + ov - 1;
    else                                   m = ov % JD_NASM;
    if (m < JD_NASM) { g_mode = (uint32_t)m; draw_frame(fb, w, h, frame); jd_audio_meter_draw(fb, w, h); return 1; }
    int sl = frame & 2047;      /* small animation clock, as the layers use */
    jd_patterns[m - JD_NASM](fb, w, h, 300 + sl, sl, mix32((uint32_t)frame >> 11), g_blend);
    jd_audio_meter_draw(fb, w, h);      /* AUDIO: HUD in single-pattern mode too */
    return 1;
}

static void motion_probe(const uint32_t *fb, int npix)
{
    int step = npix / 1024; if (step < 1) step = 1;
    uint64_t s = 0; int n = 0;
    for (int i = 0, k = 0; k < 1024 && i < npix; i += step, k++) {
        uint32_t c = fb[i], p = g_sig[k];
        int dr = (int)((c >> 16) & 255) - (int)((p >> 16) & 255);
        int dg = (int)((c >>  8) & 255) - (int)((p >>  8) & 255);
        int db = (int)( c        & 255) - (int)( p        & 255);
        s += (uint32_t)(dr < 0 ? -dr : dr) + (uint32_t)(dg < 0 ? -dg : dg)
           + (uint32_t)(db < 0 ? -db : db);
        g_sig[k] = c; n += 3;
    }
    if (!n) return;
    double d = (double)s / n;
    if (!g_sig_n) { g_sig_n = 1; return; }        /* first frame is garbage */
    g_motion = g_motion * 0.9 + d * 0.1;
}

void jd_frame(uint32_t *fb, int w, int h, int frame)
{
    if (!g_ready) {
        engine_init(fb, w, h, frame);
    } else if (w != g_w || h != g_h) {
        /* RESIZE / FULLSCREEN (review 01 F1).  This used to fall into
         * engine_init, which re-ran the whole startup: black boot fade, probe
         * sweep from scratch (~3 min with 625 routines), new bags, new run
         * seed — so every F keypress restarted the show.  Reallocate the
         * buffers and rebuild the layer palettes, and KEEP everything that
         * represents the running show: probe stats, bags, live layers, the
         * palette walk and g_run. */
        int nw = w, nh = h;
        uint32_t *nb[JD_NBUF];
        int ok = 1;
        for (int i = 0; i < JD_NBUF; i++) {
            nb[i] = (uint32_t *)calloc((size_t)nw * nh, 4);
            if (!nb[i]) ok = 0;
        }
        if (!ok) {                                  /* out of memory: bail to a full re-init */
            for (int i = 0; i < JD_NBUF; i++) free(nb[i]);
            for (int i = 0; i < JD_NBUF; i++) { free(g_buf[i]); free(g_pal[i]); }
            g_ready = 0;
            engine_init(fb, w, h, frame);
        } else {
            /* Re-point the LAYERS at the new allocations as well.  Missing
             * this left every live layer holding a dangling pointer to the
             * freed, smaller buffer while g_w/g_h advertised the new size —
             * so the first asm mode to run wrote a full new-size frame into
             * old-size memory and took the process out in draw_frame. */
            for (int i = 0; i < JD_NBUF; i++) {
                free(g_buf[i]); g_buf[i] = nb[i]; g_L[i].buf = g_buf[i];
            }
            g_w = nw; g_h = nh;
            /* layer buffers are blank now: canvas-owning layers must restart
             * their accumulation, and every live layer redraws next frame. */
            for (int i = 0; i < JD_NBUF; i++) {
                if (g_L[i].live && g_L[i].cls == C_CANVAS) g_L[i].sl = 0;
                g_lsig_ok[i] = 0;
            }
            g_sig_n = 0;                            /* motion probe re-baselines */
            for (int i = 0; i < JD_NBUF; i++) if (g_L[i].live) layer_pal_build(i);
            g_probe_pix = nw * nh;                  /* cost model follows the new area */
        }
    }
    if (mode_override(fb, w, h, frame)) return;

    double t_frame = now_ms();
    int npix = w * h;

    /* Finish measuring the library in the background of the first seconds,
     * against the frame's ACTUAL spare time rather than a fixed 2.5 ms:
     * early on the stack is one or two layers and there are 8 ms going
     * spare, and that is exactly when the sweep most needs to finish. */
    if (!g_probe_done) {
        double spare = 12.0 - g_ewma_ms;
        if (spare < 0.5) spare = 0.5;
        if (spare > 6.0) spare = 6.0;
        if (probe_step(spare)) bags_init();
        /* LAUNCH WINDOW (gate finding F1): while the sweep is still running, fold what has
         * been measured so far into the ROLE bags every 4 s, keeping each
         * bag's recent-history so the reshuffle cannot hand back what just
         * played.  Without this the bags stay as built at frame 0 (asm
         * grounds + everything else in FIGURE) until the sweep ends, and
         * with 532 routines the sweep takes ~25 s of budget = 2.5 min at
         * 60 Hz: every ground in that window is drawn from the FIGURE bag
         * under g_force, in probe order, so launches open on consecutive
         * siblings and role-blind (often dark) grounds. */
        else if (((frame - g_frame0) % 240) == 239) {
            uint16_t hist[R_NROLE][JD_SEAM]; uint32_t hpos[R_NROLE], seed[R_NROLE];
            for (int r = 0; r < R_NROLE; r++) {
                memcpy(hist[r], g_bag[r].hist, sizeof hist[r]);
                hpos[r] = g_bag[r].hpos; seed[r] = g_bag[r].seed;
            }
            bags_init();
            for (int r = 0; r < R_NROLE; r++) {
                memcpy(g_bag[r].hist, hist[r], sizeof hist[r]);
                g_bag[r].hpos = hpos[r];
                g_bag[r].seed = mix32(seed[r] ^ (uint32_t)frame);
            }
        }
    }

    palette_update(frame);

    audio_rotate();                 /* AUDIO -> COLOUR */
    audio_surge();                  /* AUDIO -> WEIGHT (eased) */
    sched_tick(frame);

    /* ---- weights ---- */
    /* A GROUND counts as present whenever it is LIVE, even on the frames its
     * eased envelope still rounds to zero.  It used to be gated on w16 != 0
     * like an overlay, and smootherstep underflows to 0 for the first ~3
     * frames of a 180-frame fade — so every fresh ground opened with a
     * two-or-three-frame window in which the engine believed it had NO
     * ground, took the !ng branch, blanked the frame and RESTARTED the boot
     * ramp.  The next 180 frames were then re-faded up from black with a
     * perfectly good picture already in the buffer: measured, composite luma
     * 0.0 against a ground buffer at luma 131.  That single line accounted
     * for most of the dark frames in the battery.  Weight is not used at all
     * when there is one ground (it is copied whole), and the two-ground path
     * normalises, so admitting a zero-weight ground here is safe. */
    int gnd[2], ng = 0, ov[JD_NSLOT], no = 0;
    for (int s = 0; s < JD_NBUF; s++) {
        jd_layer *L = &g_L[s];
        if (!L->live) { L->w_now = 0; continue; }
        L->w16   = layer_w_q16(L, frame);
        L->w_now = (uint16_t)(L->w16 >> 8);
        /* AUDIO: overlays surge with the low end (see audio_surge) — the old
         * 0.85..1.15 was measured as invisible against the music. */
        if (s != 0 && s != JD_SHADOW && L->w_now && g_surge != 256) {
            uint32_t v = ((uint32_t)L->w_now * g_surge) >> 8;
            L->w_now = (uint16_t)(v > 256 ? 256 : v);
        }
        if (s == 0 || s == JD_SHADOW) { if (ng < 2) gnd[ng++] = s; }
        else if (L->w16 && no < JD_NSLOT) ov[no++] = s;
    }
    if (!ng) {                                   /* only possible pre-cascade */
        /* Hold the boot ramp here.  It is what fades the opening up from
         * black, and it is clocked from the first jd_frame call — so if the
         * first ground is late the ramp is already 85% open when the
         * picture arrives and the arrival is a cut, not a fade (measured
         * delta 160.6).  Restart it from the first frame that has one. */
        g_frame0 = frame;
        memset(fb, 0, (size_t)npix * 4);
        return;
    }

    /* ---- render ---- */
    for (int k = 0; k < ng + no; k++) {
        int s = k < ng ? gnd[k] : ov[k - ng];
        jd_layer *L = &g_L[s];
        int falling = frame > L->t_out;
        if (falling && L->w_now < FREEZE_W && L->cls != C_CANVAS) continue;
        if (L->frozen) continue;
        if (L->half && ((frame & 1) != L->parity)) continue;
        double t0 = now_ms();
        if (L->routine < JD_NASM) {
            g_mode = (uint32_t)L->routine;
            draw_frame(L->buf, w, h, frame);
        } else {
            jd_patterns[L->routine - JD_NASM](L->buf, w, h, L->fbase + L->sl,
                                              L->sl, L->seed, L->pal + g_prot);
        }
        double dt = now_ms() - t0;
        if (dt > 20.0) TR("SLOW f=%d slot=%d rt=%d sl=%d dt=%.1f\n",
                          frame, s, L->routine, L->sl, dt);
        L->cost_ms = L->cost_ms * 0.9 + dt * 0.1;

        /* Learn what this routine's motion REALLY is, at the real
         * resolution, so admission stops trusting the startup probe. */
        {
            int st2 = npix / 512; if (st2 < 1) st2 = 1;
            uint64_t acc = 0;
            for (int q = 0; q < 512; q++) {
                uint32_t c = L->buf[q * st2], pv = g_lsig[s][q];
                int dr = (int)((c >> 16) & 255) - (int)((pv >> 16) & 255);
                int dg = (int)((c >>  8) & 255) - (int)((pv >>  8) & 255);
                int db = (int)( c        & 255) - (int)( pv        & 255);
                acc += (uint32_t)(dr < 0 ? -dr : dr) + (uint32_t)(dg < 0 ? -dg : dg)
                     + (uint32_t)(db < 0 ? -db : db);
                g_lsig[s][q] = c;
            }
            if (g_lsig_ok[s]) {
                uint32_t dq8 = (uint32_t)(acc * 256 / (512 * 3));
                L->mdel = (uint16_t)((L->mdel * 7 + dq8) >> 3);
                if (dq8 > 4096u && L->sl > 8) {
                    /* A layer that lurches a whole frame's worth in one step
                     * is a strobe, and waiting for the composite average to
                     * notice is too slow: act on this frame.  Overlays get
                     * turned down, a ground gets handed over at once. */
                    TR("JUMP f=%d slot=%d rt=%d sl=%d d=%.1f w=%d\n", frame, s,
                       L->routine, L->sl, (double)dq8 / 256.0, L->w_now);
                    g_st[L->routine].delta_q8 = (uint16_t)(dq8 > 65000 ? 65000 : dq8);
                    int sidx = (s == JD_SHADOW) ? 0 : s;
                    /* Stop calling it.  Its last frame stays on the canvas
                     * and dissolves out under the envelope — a still image
                     * fading is invisible; a routine flipping its whole
                     * composition every second is not. */
                    if (sidx != 0 || L->strikes++) L->frozen = 1;
                    if (sidx != 0 && L->w_peak > 40) {
                        /* an overlay can also be turned down; frozen and
                         * quiet, it is just a still highlight dissolving */
                        L->w_peak = (uint16_t)(L->w_peak * 5 / 8);
                    } else if (sidx == 0 && L->t_out > frame + 60) {
                        /* A GROUND keeps its envelope: crushing a ground's
                         * peak leaves the ground blend deciding between two
                         * near-zero weights, and that ratio is quantisation
                         * noise — which is a hard cut of the whole frame.
                         * Freeze it and bring its handover forward instead. */
                        L->t_out = frame + 60 > L->t_full ? frame + 60 : L->t_full;
                        L->t_end = L->t_out + FADE_OUT[0];
                    }
                }
            }
            if (g_lsig_ok[s] && L->sl > 60) {
                uint32_t dq8 = (uint32_t)(acc * 256 / (512 * 3));
                uint32_t cur = g_st[L->routine].delta_q8;
                cur = dq8 > cur ? cur + ((dq8 - cur) >> 2)   /* attack */
                                : cur - ((cur - dq8) >> 5);  /* decay  */
                if (cur > 65000) cur = 65000;
                g_st[L->routine].delta_q8 = (uint16_t)cur;
            }
            g_lsig_ok[s] = 1;
        }
        g_st[L->routine].cost_q8 = (uint16_t)(L->cost_ms * 256.0 > 65000.0
                                              ? 65000.0 : L->cost_ms * 256.0);
        L->sl++;
    }

    /* ---- dark-ground guard: a ground whose OWN buffer goes near-black (the
     * palette leg walked its index band into a dark stretch of the ramp) is
     * dead air the composite guard cannot see while overlays hold luma ~30.
     * Bring its handover forward, exactly as CALM does — nothing jumps. */
    if (g_L[0].live && !g_L[JD_SHADOW].live && g_L[0].sl > 60) {
        int st = npix / 512; if (st < 1) st = 1;
        uint32_t la = 0;
        for (int q = 0; q < 512; q++) {
            uint32_t ca = g_L[0].buf[q * st];
            la += (((ca >> 16) & 255) * 77 + ((ca >> 8) & 255) * 151 + (ca & 255) * 28) >> 8;
        }
        la /= 512;
        g_gluma = (g_gluma * 31 + la) >> 5;                  /* ~0.5 s EWMA */
        if (g_gluma < 20 && g_L[0].t_out > frame + 240) {
            TR("DIM f=%d rt=%d gluma=%u -> handover forward\n", frame, g_L[0].routine, g_gluma);
            g_L[0].t_out = frame + 60; g_L[0].t_end = g_L[0].t_out + FADE_OUT[0];
        }
    } else if (g_L[0].live && g_L[0].sl <= 60) g_gluma = 60;
    if (ng == 2 && g_L[JD_SHADOW].live && g_L[JD_SHADOW].sl == 6 && g_audition < 2) {
        int st = npix / 512; if (st < 1) st = 1;
        uint32_t la = 0, lb = 0;
        for (int q = 0; q < 512; q++) {
            uint32_t ca = g_L[0].buf[q * st], cb = g_L[JD_SHADOW].buf[q * st];
            la += (((ca >> 16) & 255) * 77 + ((ca >> 8) & 255) * 151 + (ca & 255) * 28) >> 8;
            lb += (((cb >> 16) & 255) * 77 + ((cb >> 8) & 255) * 151 + (cb & 255) * 28) >> 8;
        }
        la /= 512; lb /= 512;
        if (lb < 18 && la > 3 * lb) {
            TR("AUDITION f=%d rt=%d luma=%u (ground %u) -> redraw\n", frame, g_L[JD_SHADOW].routine, lb, la);
            g_L[JD_SHADOW].live = 0; g_L[JD_SHADOW].w16 = 0; g_L[JD_SHADOW].w_now = 0;
            ng = 1; g_audition++;
        }
    }
    /* ---- ground: normalized, so the frame never pulses ---- */
    if (ng == 2) {
        uint64_t a = g_L[gnd[0]].w16, b = g_L[gnd[1]].w16;
        /* both envelopes can round to zero on a handover's first frames; the
         * incumbent holds the screen until one of them has any weight */
        if (!(a + b)) memcpy(fb, g_L[gnd[0]].buf, (size_t)npix * 4);
        else {
            uint32_t wb = (uint32_t)(b * 256 / (a + b));
            span_lerp(fb, g_L[gnd[0]].buf, g_L[gnd[1]].buf, npix, wb);
        }
    } else {
        memcpy(fb, g_L[gnd[0]].buf, (size_t)npix * 4);
    }

    /* ---- overlays, bottom up ---- */
    for (int k = 0; k < no; k++) {
        jd_layer *L = &g_L[ov[k]];
        blend_span(fb, L->buf, npix, L->w_now, L->blend);
    }

    /* ---- boot: ease up over the first 2 s, FROM A FLOOR ----
     * A 3 s smootherstep from zero spends its first second under 10% — the
     * app opened on a black screen and the QA battery scored the opening as
     * dead frames (luma 1.1 at 0.5 s, 16.0 at 1 s).  Start at 19% instead:
     * still unmistakably a fade-up, but the picture is there from frame one. */
    if (frame - g_frame0 < JD_BOOT) {
        uint32_t e  = ease_ss((uint32_t)(((int64_t)(frame - g_frame0) << 16) / JD_BOOT));
        uint32_t bw = 48 + ((e * 208) >> 16);
        if (bw < 256) span_scale(fb, fb, npix, bw);
    }

    /* ---- dead-air guard: measured as low as luma 6.8/255 while an
     * accumulator was still drawing its first strokes.  Sparse-sample the
     * composite and lift it toward a floor with an eased gain (attack 1/32,
     * release 1/8) so the correction never pulses.  Dark-but-alive frames
     * are untouched: the moody stretches are approved content. */
    {
        int st = npix / 512; if (st < 1) st = 1;
        uint32_t sum = 0;
        for (int q = 0; q < 512; q++) {
            uint32_t c = fb[q * st];
            sum += (((c >> 16) & 255) * 77 + ((c >> 8) & 255) * 151
                    + (c & 255) * 28) >> 8;
        }
        uint32_t luma = sum / 512, want = 256;
        if (luma < JD_LUMA_FLOOR) {
            want = (JD_LUMA_FLOOR * 256) / (luma < 4 ? 4 : luma);
            if (want > JD_LUMA_MAXGAIN) want = JD_LUMA_MAXGAIN;
        }
        g_gain = want > g_gain ? g_gain + ((want - g_gain + 31) >> 5)
                               : g_gain - ((g_gain - want + 7) >> 3);
        if (g_gain > 260) span_gain(fb, fb, npix, g_gain);
    }

#ifdef JD_INSTR
    /* TEMPORARY dark-frame instrumentation — removed before release */
    {
        double L = 0; for (int i = 0; i < npix; i++)
            L += ((fb[i]>>16&255)*.30 + (fb[i]>>8&255)*.59 + (fb[i]&255)*.11);
        L /= npix;
        static const char *RN[] = {"GND","FLD","FIG","SPK"};
        if (L < 40.0) {
            fprintf(stderr, "DARKF f=%d luma=%.1f ng=%d no=%d gain=%u mood=%d\n",
                    frame, L, ng, no, g_gain, g_mood);
            for (int s = 0; s < JD_NBUF; s++) {
                jd_layer *Q = &g_L[s];
                if (!Q->live) continue;
                double bl = 0; for (int i = 0; i < npix; i += 7)
                    bl += ((Q->buf[i]>>16&255)*.30 + (Q->buf[i]>>8&255)*.59 + (Q->buf[i]&255)*.11);
                bl /= (npix + 6) / 7;
                fprintf(stderr, "   s=%d rt=%3d role=%s cls=%s w=%3d peak=%3d bl=%d frz=%d "
                        "sl=%4d buflum=%6.1f pluma=%3d pdark=%3d t[%d %d %d %d]\n",
                        s, Q->routine, RN[g_st[Q->routine].role],
                        g_st[Q->routine].cls == C_CANVAS ? "CANV" : "PURE",
                        Q->w_now, Q->w_peak, Q->blend, Q->frozen, Q->sl, bl,
                        g_st[Q->routine].luma, g_st[Q->routine].dark,
                        Q->t_in, Q->t_full, Q->t_out, Q->t_end);
            }
        }
    }
#endif

    /* ---- AUDIO: bloom on the beat, capped at ~1.18x and eased through the
     * same gain the dead-air guard uses: a pulse of light, never a strobe. */
    if (g_audio.live) {
        /* bass carries a sustained lift, the beat adds a kick on top:
         * up to ~1.45x, eased, so the picture pumps with the track. */
        /* SEIZURE SAFETY (J: "leaning toward a flash"): brightness barely
         * moves — 1.06x at most, and it EASES in over ~8 frames and out over
         * ~32.  The music is carried by colour rotation and by layers
         * surging, neither of which flashes. */
        uint32_t bump = 256 + (((uint32_t)g_audio.bass * 10
                              + (uint32_t)g_audio.beat * 6) >> 10);
        if (bump > 272) bump = 272;
        if (bump > g_gain) g_gain += (bump - g_gain) >> 3;
        else               g_gain -= (g_gain - bump) >> 5;
        if (g_gain > 260) span_gain(fb, fb, npix, g_gain);
    }

    /* ---- health: frame time and composite motion ---- */
    motion_probe(fb, npix);

    /* If the composite is moving too fast, ADDING layers calms it (MAX over
     * a busy ground dilutes its motion), so never block spawns on motion —
     * instead bring the jumpiest layer's exit forward.  Blocking spawns here
     * deadlocks: a fast solo ground would keep the motion high forever and
     * nothing would ever be admitted to temper it. */
    if (g_motion > 5.5) {
        if (++g_jitter > 30) {
            g_jitter = 0;
            /* Blame the layer that is actually moving, measured this tenancy
             * and weighted by how much of it is on screen — not the routine's
             * table value, which is an average over seeds. */
            int worst = -1; uint32_t wd = 0;
            for (int s = 0; s < JD_NBUF; s++) {
                if (!g_L[s].live) continue;
                uint32_t c = ((uint32_t)g_L[s].mdel * g_L[s].w_now) >> 8;
                if (c > wd) { wd = c; worst = s; }
            }
            if (worst >= 0) {
                jd_layer *L = &g_L[worst];
                int sidx = (worst == JD_SHADOW) ? 0 : worst;
                if (sidx != 0 && L->w_peak > 40) {
                    /* an overlay can simply be turned down — the envelope
                     * keeps it smooth, it just stops shouting */
                    L->w_peak = (uint16_t)(L->w_peak * 7 / 8);
                    TR("TRIM f=%d slot=%d rt=%d mdel=%u peak=%u\n",
                       frame, worst, L->routine, L->mdel, L->w_peak);
                } else if (L->t_out > frame + 60) {
                    L->t_out = frame + 60;
                    L->t_end = L->t_out + FADE_OUT[sidx];
                    TR("CALM f=%d slot=%d rt=%d mdel=%u\n", frame, worst, L->routine, L->mdel);
                }
            }
        }
    } else g_jitter = 0;

    /* AUDIO: the on-screen level meter (key M) goes on LAST, after every
     * gain and after the health probes, so it is never brightened, never
     * counted as motion, and always legible. */
    jd_audio_meter_draw(fb, w, h);

    TRF("F %d n=%d/%d w=%d,%d,%d,%d,%d A=%d key=%d p0=%08x b0=%08x sl0=%d rt0=%d\n",
        frame, ng, no,
        g_L[0].w_now, g_L[1].w_now, g_L[2].w_now, g_L[3].w_now, g_L[4].w_now,
        g_blend_key >> 17, g_blend_key, g_pal[0][1000], g_blend[1000],
        g_L[0].sl, g_L[0].routine);
    double ms = now_ms() - t_frame;
    g_ewma_ms = g_ewma_ms * 0.92 + ms * 0.08;
    if (!g_hot && g_ewma_ms > 13.5) {
        g_hot = 1; g_cool = 0;
        if (no) {                                  /* half-rate the top layer */
            g_L[ov[no - 1]].half = 1;
            g_L[ov[no - 1]].parity = (uint8_t)(frame & 1);
        }
    } else if (g_hot) {
        if (g_ewma_ms < 10.5) { if (++g_cool > 120) {
            g_hot = 0;
            for (int s = 1; s < JD_NSLOT; s++) g_L[s].half = 0;
        } } else g_cool = 0;
    }
}
