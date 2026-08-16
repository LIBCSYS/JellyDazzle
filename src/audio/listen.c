/* listen.c — JellyDazzleAudio: what the Mac is playing -> dance.
 *
 * Captures the ACTIVE audio — first choice is the system output itself
 * (a Core Audio process tap: Spotify, YouTube, anything, exactly as mixed,
 * before the volume knob, speakers or headphones alike — see systap.m);
 * the built-in microphone is the fallback — and turns it into a handful of
 * smooth numbers for the engine:
 *
 *     bass / mid / treble   band energies, 0..1024, attack-fast release-slow
 *     level                 overall loudness, 0..1024
 *     beat                  a pulse that spikes to 1024 on an onset and decays
 *     bpm_q8                running tempo estimate (Q8/4), 0 if unsure
 *     live                  1 while real sound is arriving
 *
 * Source selection (JD_AUDIO_SRC overrides: "tap", "mic", or a device index):
 *   1. system output tap      macOS 14.2+, no install, no permission prompt
 *                             measured on 26.6 (a bundled .app should carry
 *                             NSAudioCaptureUsageDescription so the prompt
 *                             can appear if TCC wants one)
 *   2. built-in microphone    "MacBook Pro Microphone" by name
 *   3. any non-virtual input  then the system default
 *
 * THE TRAP (documented because it cost a day): the system DEFAULT input can be
 * a virtual device (Teams, Zoom, BlackHole with nothing routed into it, an
 * iPhone continuity mic that is asleep) that delivers digital silence forever
 * — the callback fires 800 times with every sample 0.0000.  That looks
 * exactly like a denied microphone permission.  So we never trust "default":
 * we pick the tap or a real mic by name, and the debug log always prints the
 * callback count AND the loudest sample seen so silence-with-callbacks is
 * distinguishable from no-callbacks at a glance.
 *
 * Design rules, learned from this project's whole history:
 *   - NOTHING STROBES.  Every value is enveloped (fast attack, slow release)
 *     and the visuals read these as targets, not as instant jumps.
 *   - SILENCE IS FINE.  With no signal the values decay to zero and the
 *     engine's own clocks carry the show exactly as before.
 *   - CHEAP.  A 2048-point real FFT once per frame is ~60 us; the capture
 *     callback only copies into a ring buffer.
 *   - LOUDNESS-INDEPENDENT.  A slow automatic gain reference (20 s release)
 *     means a quiet track and a loud one both use the whole 0..1024 range;
 *     an absolute floor keeps silence and hiss at zero.
 *
 * On-screen meter: key M toggles a small HUD (bass/mid/treble/level bars, a
 * beat dot, source and BPM) drawn straight into the framebuffer by
 * jd_audio_meter_draw(), which the compositor calls last.  JD_AUDIO_METER=1
 * starts with it on.  JD_AUDIO_DEBUG=1 appends the meter to
 * /tmp/jd_audio_meter.log four times a second.
 */
#include <SDL.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "jellydazzle.h"
extern const char *jd_role_name(int role);

#define AU_N      2048                 /* FFT size: 42.7 ms @ 48k, 23 Hz/bin */
#define AU_RING   (AU_N * 4)

jd_audio g_audio;                      /* the public, read-only-ish state   */

/* ---- systap.m (Objective-C, Core Audio process tap) ---- */
typedef void (*jd_tap_push_fn)(const float *samples, int n, int stride);
extern int  jd_systap_open(jd_tap_push_fn push, int *rate_out, char *name, int namelen);
extern void jd_systap_close(void);
extern int  jd_systap_changed(void);
extern const char *jd_systap_error(void);

enum { SRC_NONE = 0, SRC_TAP, SRC_MIC };
static int      au_src = SRC_NONE;
static char     au_srcname[96];      /* device / clock name for the HUD */
static int      au_tap_retry;          /* auto mode: tap retries left       */
static int      au_tap_retry_at;
static int      au_rate = 48000;
static int      au_want_src;           /* from JD_AUDIO_SRC: 0 auto, else SRC_* */
static int      au_want_dev = -1;      /* JD_AUDIO_SRC=<n>: SDL input index    */

static float    au_ring[AU_RING];
static volatile int au_w;              /* ring write cursor                 */
static SDL_AudioDeviceID au_dev;
static int      au_on;

static float    au_win[AU_N];          /* Hann window                       */
static float    au_prev[AU_N / 2];     /* previous log-magnitudes (flux)    */
static float    au_flux_hist[32];      /* recent flux for adaptive threshold */
static int      au_flux_i;
static float    au_flux_l1, au_flux_l2;/* last two flux values (peak pick)  */
static int      au_since_beat;
static int      au_beat_gap[8], au_gap_i;
static float    au_ref;                /* AGC reference (slow peak follower)*/
static float    au_floor = 0.03f;      /* AGC floor per source              */
static float    au_live_thr = 0.001f;  /* absolute "real signal" threshold  */

/* meter state */
static int      au_meter;              /* HUD on/off                        */
static int      au_meter_key_was;
static int      au_about;              /* ABOUT card on/off                 */
int             au_skip;               /* SPACE: dismiss the first-run card  */
static int      au_skip_key_was;
static int      au_about_key_was;
static uint16_t au_peak_hold[4];       /* slow-falling peak ticks per bar   */
static int      au_stale;              /* ticks with no new callbacks       */
static float    au_rms_now;

/* ---- capture callbacks: copy in, do no work ---- */
static volatile int au_cbs;        /* callbacks seen        */
static volatile float au_peak;     /* loudest sample seen   */

static void au_push(const float *in, int n, int stride)
{
    au_cbs++;
    int w = au_w;
    for (int i = 0; i < n; i++) {
        float s = in[i * stride];
        float v = s < 0 ? -s : s;
        if (v > au_peak) au_peak = v;
        au_ring[w] = s;
        w = (w + 1) & (AU_RING - 1);
    }
    au_w = w;
}

static void au_sdl_cb(void *ud, Uint8 *stream, int len)
{
    (void)ud;
    au_push((const float *)stream, len / (int)sizeof(float), 1);
}

/* ---- tiny in-place radix-2 FFT (real input, complex scratch) ---- */
static void au_fft(float *re, float *im, int n)
{
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            float t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * (float)M_PI / (float)len;
        float wr = cosf(ang), wi = sinf(ang);
        for (int i = 0; i < n; i += len) {
            float cr = 1.0f, ci = 0.0f;
            for (int k = 0; k < len / 2; k++) {
                float ur = re[i + k],           ui = im[i + k];
                float vr = re[i + k + len / 2] * cr - im[i + k + len / 2] * ci;
                float vi = re[i + k + len / 2] * ci + im[i + k + len / 2] * cr;
                re[i + k] = ur + vr;  im[i + k] = ui + vi;
                re[i + k + len / 2] = ur - vr;  im[i + k + len / 2] = ui - vi;
                float nr = cr * wr - ci * wi;
                ci = cr * wi + ci * wr;  cr = nr;
            }
        }
    }
}

static void au_log(const char *fmt, ...)
{
    FILE *lf = fopen("/tmp/jd_audio_meter.log", "a");
    if (!lf) return;
    va_list ap; va_start(ap, fmt); vfprintf(lf, fmt, ap); va_end(ap);
    fclose(lf);
}

/* ---- source 1: system output tap ---- */
static int au_open_tap(void)
{
    int rate = 48000;
    char nm[64] = "";
    if (!jd_systap_open(au_push, &rate, nm, sizeof nm)) {
        au_log("tap: unavailable — %s (denied in Privacy & Security > Screen & "
               "System Audio Recording, or no output device)\n", jd_systap_error());
        return 0;
    }
    au_rate = rate;
    au_src = SRC_TAP;
    au_floor = 0.012f;         /* -38 dBFS: Spotify at 30% still fills the range */
    au_live_thr = 0.0008f;     /* anything above digital silence is "live" */
    snprintf(au_srcname, sizeof au_srcname, "%s", nm);
    au_log("source=TAP (system output via Core Audio process tap) clock=\"%s\" rate=%d\n",
           nm, rate);
    return 1;
}

/* ---- source 2: a REAL microphone via SDL ---- */
static int au_open_mic(void)
{
    if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        SDL_Log("JellyDazzleAudio: audio subsystem unavailable (%s)", SDL_GetError());
        return 0;
    }
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq     = 48000;
    want.format   = AUDIO_F32SYS;
    want.channels = 1;
    want.samples  = 512;
    want.callback = au_sdl_cb;

    const char *want_name = NULL;
    int ndev = SDL_GetNumAudioDevices(1);
    if (au_want_dev >= 0 && au_want_dev < ndev)
        want_name = SDL_GetAudioDeviceName(au_want_dev, 1);
    if (!want_name) {
        for (int i = 0; i < ndev && !want_name; i++) {
            const char *n = SDL_GetAudioDeviceName(i, 1);
            if (n && SDL_strstr(n, "MacBook") && SDL_strstr(n, "Micro"))
                want_name = n;
        }
    }
    if (!want_name) {
        /* never a virtual driver: those are the digital-silence trap */
        static const char *virt[] = { "Teams", "Zoom", "BlackHole", "Loopback",
                                      "Aggregate", "Soundflower", "iPhone", NULL };
        for (int i = 0; i < ndev && !want_name; i++) {
            const char *n = SDL_GetAudioDeviceName(i, 1);
            int bad = 0;
            for (int v = 0; virt[v] && n; v++) if (SDL_strstr(n, virt[v])) bad = 1;
            if (n && !bad) want_name = n;
        }
    }
    au_dev = SDL_OpenAudioDevice(want_name, 1, &want, &have, 0);
    if (!au_dev) au_dev = SDL_OpenAudioDevice(NULL, 1, &want, &have, 0);
    au_log("mic: device=\"%s\" of %d\n", want_name ? want_name : "(system default)", ndev);
    if (!au_dev) {
        SDL_Log("JellyDazzleAudio: no microphone (%s) — running on internal "
                "clocks", SDL_GetError());
        return 0;
    }
    au_log("mic: opened dev=%d freq=%d ch=%d fmt=0x%x samples=%d\n",
           (int)au_dev, have.freq, have.channels, have.format, have.samples);
    au_rate = have.freq > 1000 ? have.freq : 48000;
    au_src = SRC_MIC;
    au_floor = 0.006f;         /* measured: room speech is rms ~0.0034 */
    au_live_thr = 0.0012f;     /* above the MacBook mic's room hiss */
    snprintf(au_srcname, sizeof au_srcname, "%s", want_name ? want_name : "default");
    au_log("source=MIC \"%s\" rate=%d\n", want_name ? want_name : "default", au_rate);
    SDL_PauseAudioDevice(au_dev, 0);
    return 1;
}

static void au_close_src(void)
{
    if (au_src == SRC_TAP) jd_systap_close();
    if (au_src == SRC_MIC && au_dev) { SDL_CloseAudioDevice(au_dev); au_dev = 0; }
    au_src = SRC_NONE;
    au_stale = 0;
}

static int au_open_any(void)
{
    if (au_want_src == SRC_MIC) return au_open_mic();
    if (au_want_src == SRC_TAP) return au_open_tap();
    return au_open_tap() || au_open_mic();
}

int jd_audio_init(void)
{
    if (au_on) return 1;
    for (int i = 0; i < AU_N; i++)
        au_win[i] = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * i / (AU_N - 1));

    /* HUD flags are read BEFORE any early return: the meter and the about card
     * are not audio features, and JD_AUDIO_SRC=off used to disable them as a
     * side effect of bailing out of this function. */
    if (getenv("JD_AUDIO_METER")) au_meter = 1;
    if (getenv("JD_ABOUT")) au_about = 1;

    const char *e = getenv("JD_AUDIO_SRC");
    if (e) {
        if (!SDL_strcasecmp(e, "off") || !SDL_strcasecmp(e, "none")) {
            au_log("source=OFF (JD_AUDIO_SRC=off) — internal clocks only\n");
            return 0;                       /* the engine runs on its clocks */
        }
        if (!SDL_strcasecmp(e, "tap")) au_want_src = SRC_TAP;
        else if (!SDL_strcasecmp(e, "mic")) au_want_src = SRC_MIC;
        else if (!SDL_strncasecmp(e, "dev:", 4)) { au_want_src = SRC_MIC; au_want_dev = SDL_atoi(e + 4); }
        else if (e[0] >= '0' && e[0] <= '9') { au_want_src = SRC_MIC; au_want_dev = SDL_atoi(e); }
    }
    /* legacy knob from 2.3 */
    if ((e = getenv("JD_AUDIO_DEV")) != NULL) { au_want_src = SRC_MIC; au_want_dev = SDL_atoi(e); }
    au_ref = au_floor;
    if (!au_open_any()) return 0;
    au_ref = au_floor;
    /* auto mode landed on the mic: keep trying the tap for a minute (a
     * device that was mid-switch at launch usually comes good) */
    if (au_want_src == 0 && au_src == SRC_MIC) { au_tap_retry = 12; au_tap_retry_at = 300; }
    au_on = 1;
    return 1;
}

void jd_audio_close(void)
{
    if (au_on) { au_close_src(); au_on = 0; }
}

/* envelope: fast attack, slow release, so nothing flickers */
static uint16_t env(uint16_t cur, float target_f, int att_sh, int rel_sh)
{
    int t = (int)(target_f > 1.0f ? 1024.0f : target_f * 1024.0f);
    if (t < 0) t = 0;
    if (t > cur) return (uint16_t)(cur + ((t - cur) >> att_sh));
    if (t == cur) return cur;
    int d = (cur - t) >> rel_sh;
    return (uint16_t)(cur - (d ? d : 1));       /* always reaches 0 in silence */
}

static void au_decay_all(void)
{
    g_audio.level = (uint16_t)(g_audio.level * 15 / 16);
    g_audio.bass  = (uint16_t)(g_audio.bass  * 15 / 16);
    g_audio.mid   = (uint16_t)(g_audio.mid   * 15 / 16);
    g_audio.treble= (uint16_t)(g_audio.treble* 15 / 16);
    g_audio.beat  = (uint16_t)(g_audio.beat  * 7 / 8);
    g_audio.live  = 0;
}

void jd_audio_tick(void)
{
    /* M toggles the meter — polled here so main.c needs no change; the
     * event loop has already pumped this frame's keys. */
    {
        const Uint8 *ks = SDL_GetKeyboardState(NULL);
        int m = ks ? ks[SDL_SCANCODE_M] : 0;
        if (m && !au_meter_key_was) au_meter ^= 1;
        au_meter_key_was = m;
        int a = ks ? ks[SDL_SCANCODE_A] : 0;      /* A toggles the about card */
        if (a && !au_about_key_was) au_about ^= 1;
        au_about_key_was = a;
        int k = ks ? (ks[SDL_SCANCODE_SPACE] || ks[SDL_SCANCODE_RETURN]) : 0;
        if (k && !au_skip_key_was) au_skip = 1;   /* dismiss, never un-dismiss */
        au_skip_key_was = k;
    }

    if (!au_on) { au_decay_all(); return; }

    if (au_src == SRC_MIC && au_tap_retry > 0 && --au_tap_retry_at <= 0) {
        au_tap_retry--; au_tap_retry_at = 300;
        SDL_AudioDeviceID keep = au_dev;
        if (au_open_tap()) {              /* switch: tap wins over the room */
            if (keep) SDL_CloseAudioDevice(keep);
            au_dev = 0; au_ref = au_floor; au_tap_retry = 0;
            au_log("tap: came good on retry, leaving the mic\n");
        } else au_src = SRC_MIC;
    }

    /* headphones on/off: the tap's clock device moved — rebuild it */
    if (au_src == SRC_TAP && jd_systap_changed()) {
        au_log("tap: default output changed, rebuilding\n");
        au_close_src();
        if (!au_open_tap() && !au_open_mic()) { au_on = 0; au_decay_all(); return; }
    }

    /* no callbacks for 0.25 s = the device is idle (Bluetooth output with
     * nothing playing stops clocking): that is silence, not a held note */
    static int last_cbs = -1;
    if (au_cbs == last_cbs) { if (au_stale < 1000) au_stale++; }
    else { au_stale = 0; last_cbs = au_cbs; }
    int idle = au_stale > 15;

    /* newest AU_N samples out of the ring */
    static float re[AU_N], im[AU_N];
    int w = au_w;
    if (idle) {
        memset(re, 0, sizeof re); memset(im, 0, sizeof im);
    } else {
        for (int i = 0; i < AU_N; i++) {
            int idx = (w - AU_N + i) & (AU_RING - 1);
            re[i] = au_ring[idx] * au_win[i];
            im[i] = 0.0f;
        }
    }

    /* rms of the windowed block, corrected for the Hann power loss (0.375) */
    float rms = 0.0f;
    for (int i = 0; i < AU_N; i++) rms += re[i] * re[i];
    rms = sqrtf(rms / (AU_N * 0.375f));
    au_rms_now = rms;

    /* AGC reference: jumps up to a louder passage in ~3 frames, relaxes
     * over ~20 s, never below the source floor */
    if (rms > au_ref) au_ref += (rms - au_ref) * 0.4f;
    else              au_ref -= (au_ref - rms) * 0.0008f;
    if (au_ref < au_floor) au_ref = au_floor;
    float ref = au_ref;

    au_fft(re, im, AU_N);

    /* band edges from Hz, so 44.1k and 48k sources agree */
    float hzb = (float)au_rate / AU_N;
    int kb0 = (int)(40.0f / hzb), kb1 = (int)(200.0f / hzb);     /* bass  */
    int km1 = (int)(2000.0f / hzb);                               /* mid   */
    int kt1 = (int)(12000.0f / hzb);                              /* treble*/
    int kf1 = (int)(5000.0f / hzb);                               /* flux  */
    if (kb0 < 1) kb0 = 1;
    if (kt1 > AU_N / 2 - 1) kt1 = AU_N / 2 - 1;

    float e_bass = 0, e_mid = 0, e_tre = 0, flux = 0;
    for (int k = 1; k < AU_N / 2; k++) {
        float p = re[k] * re[k] + im[k] * im[k];
        if      (k >= kb0 && k <= kb1) e_bass += p;
        else if (k >  kb1 && k <= km1) e_mid  += p;
        else if (k >  km1 && k <= kt1) e_tre  += p;
        if (k <= kf1) {
            /* onset strength on a compressed, loudness-relative spectrum,
             * weighted toward the low end where the beat lives */
            float m = sqrtf(p) / (ref * (AU_N * 0.25f));
            float lm = logf(1.0f + 8.0f * m);
            float d = lm - au_prev[k];
            au_prev[k] = lm;
            if (d > 0) flux += d * (k <= kb1 ? 2.0f : (k <= km1 ? 1.0f : 0.5f));
        }
    }
    /* band rms (Parseval: 2 sum|X|^2 / (N * sum w^2)) relative to the AGC
     * reference; the per-band gains reflect how music's energy is really
     * split (bass carries most of it) so the three bars sit level for a
     * typical track and each one swings with its own instruments */
    float nrm = 2.0f / ((float)AU_N * (float)AU_N * 0.375f);
    float r_bass = sqrtf(e_bass * nrm), r_mid = sqrtf(e_mid * nrm), r_tre = sqrtf(e_tre * nrm);
    float f_level = rms / ref * 1.15f;
    float f_bass  = r_bass / ref * 1.70f;
    float f_mid   = r_mid  / ref * 1.70f;
    float f_tre   = r_tre  / ref * 4.00f;
    /* below the absolute floor nothing counts: hiss and dither stay at 0 */
    if (rms < au_live_thr) f_level = f_bass = f_mid = f_tre = 0.0f;

    g_audio.level  = env(g_audio.level,  f_level, 1, 4);   /* 2 up / 16 down */
    g_audio.bass   = env(g_audio.bass,   f_bass,  1, 3);   /* kicks: punchy   */
    g_audio.mid    = env(g_audio.mid,    f_mid,   1, 4);
    g_audio.treble = env(g_audio.treble, f_tre,   1, 4);
    /* live engages on real sound and lets go ~1 s after it stops */
    static int live_hold;
    if (rms >= au_live_thr) live_hold = 60; else if (live_hold) live_hold--;
    g_audio.live = (uint16_t)(live_hold > 0);

    /* ---- onset detection: flux is a local peak above an adaptive mean of
     * the last half second, with a refractory gap so a sustained note is
     * not a drum roll ---- */
    au_flux_hist[au_flux_i++ & 31] = flux;
    float mean = 0; for (int i = 0; i < 32; i++) mean += au_flux_hist[i];
    mean *= (1.0f / 32.0f);
    au_since_beat++;
    int onset = au_flux_l1 > mean * 1.7f + 0.8f && au_flux_l1 >= flux
                && au_flux_l1 > au_flux_l2 && au_since_beat > 8 && g_audio.live
                && rms >= au_live_thr;
    au_flux_l2 = au_flux_l1; au_flux_l1 = flux;
    static int beats_seen;
    if (onset) {
        g_audio.beat = 1024; beats_seen++;
        au_beat_gap[au_gap_i++ & 7] = au_since_beat;
        au_since_beat = 0;
        /* median-ish tempo from the last 8 gaps (frames -> BPM, 60 fps) */
        int s = 0, n = 0;
        for (int i = 0; i < 8; i++) if (au_beat_gap[i]) { s += au_beat_gap[i]; n++; }
        if (n >= 4) {
            float gap = (float)s / n;               /* frames between beats */
            float bpm = 3600.0f / gap;              /* 60 fps * 60 s        */
            while (bpm > 190.0f) bpm *= 0.5f;       /* off-beats double it  */
            if (bpm > 60.0f && bpm < 190.0f)
                g_audio.bpm_q8 = (uint16_t)(bpm * 256.0f / 4.0f); /* Q8/4 */
        }
    } else {
        g_audio.beat = (uint16_t)(g_audio.beat * 7 / 8);   /* ~0.2 s decay */
    }
    if (!g_audio.live) g_audio.bpm_q8 = 0;

    /* peak-hold ticks for the HUD: rise at once, fall slowly */
    {
        uint16_t v[4] = { g_audio.bass, g_audio.mid, g_audio.treble, g_audio.level };
        for (int i = 0; i < 4; i++) {
            if (v[i] > au_peak_hold[i]) au_peak_hold[i] = v[i];
            else if (au_peak_hold[i] > 6) au_peak_hold[i] -= 6; else au_peak_hold[i] = 0;
        }
    }

    /* JD_AUDIO_DEBUG=1 prints the meter ~4x a second, for tuning by ear */
    {
        static int dbg = -1, tick;
        if (dbg < 0) dbg = getenv("JD_AUDIO_DEBUG") ? 1 : 0;
        if (dbg && (++tick % 15) == 0) {
            static int beats_last;
            au_log("src=%s cb=%d peak=%.4f rms=%.4f ref=%.4f rb=%.2f rm=%.2f rt=%.2f "
                   "flux=%.2f/%.2f nb=%d idle=%d live=%d "
                   "level=%4d bass=%4d mid=%4d treble=%4d beat=%4d bpm=%d\n",
                   au_src == SRC_TAP ? "tap" : au_src == SRC_MIC ? "mic" : "none",
                   au_cbs, au_peak, rms, ref,
                   rms > 0 ? r_bass / rms : 0, rms > 0 ? r_mid / rms : 0, rms > 0 ? r_tre / rms : 0,
                   flux, mean, beats_seen - beats_last, idle,
                   g_audio.live, g_audio.level, g_audio.bass, g_audio.mid,
                   g_audio.treble, g_audio.beat, g_audio.bpm_q8 * 4 / 256);
            beats_last = beats_seen;
            au_peak = 0;
        }
    }
}

/* ============================================================
 * On-screen meter (key M)
 * ============================================================ */

/* 3x5 pixel font: rows top->bottom, 3 bits each, MSB left */
static const char *au_glyph(char c)
{
    switch (c) {
    case '0': return "111101101101111"; case '1': return "010110010010111";
    case '2': return "111001111100111"; case '3': return "111001111001111";
    case '4': return "101101111001001"; case '5': return "111100111001111";
    case '6': return "111100111101111"; case '7': return "111001001001001";
    case '8': return "111101111101111"; case '9': return "111101111001111";
    case 'A': return "010101111101101"; case 'B': return "110101110101110";
    case 'C': return "111100100100111"; case 'D': return "110101101101110";
    case 'E': return "111100111100111"; case 'F': return "111100111100100";
    case 'G': return "111100101101111"; case 'H': return "101101111101101";
    case 'I': return "111010010010111"; case 'J': return "001001001101111";
    case 'K': return "101101110101101"; case 'L': return "100100100100111";
    case 'M': return "101111111101101"; case 'N': return "110101101101101";
    case 'O': return "111101101101111"; case 'P': return "111101111100100";
    case 'Q': return "111101101111001"; case 'R': return "110101110101101";
    case 'S': return "111100111001111"; case 'T': return "111010010010010";
    case 'U': return "101101101101111"; case 'V': return "101101101101010";
    case 'W': return "101101111111101"; case 'X': return "101101010101101";
    case 'Y': return "101101010010010"; case 'Z': return "111001010100111";
    case ':': return "000010000010000"; case '-': return "000000111000000";
    case '.': return "000000000000010"; case '/': return "001001010100100";
    default:  return NULL;
    }
}

static void au_rect(uint32_t *fb, int w, int h, int x0, int y0, int rw, int rh, uint32_t c)
{
    if (x0 < 0) { rw += x0; x0 = 0; }
    if (y0 < 0) { rh += y0; y0 = 0; }
    if (x0 + rw > w) rw = w - x0;
    if (y0 + rh > h) rh = h - y0;
    for (int y = 0; y < rh; y++) {
        uint32_t *row = fb + (size_t)(y0 + y) * w + x0;
        for (int x = 0; x < rw; x++) row[x] = c;
    }
}

/* darken a rectangle to ~35% so the HUD reads on any picture */
static void au_dim(uint32_t *fb, int w, int h, int x0, int y0, int rw, int rh)
{
    if (x0 < 0) { rw += x0; x0 = 0; }
    if (y0 < 0) { rh += y0; y0 = 0; }
    if (x0 + rw > w) rw = w - x0;
    if (y0 + rh > h) rh = h - y0;
    for (int y = 0; y < rh; y++) {
        uint32_t *row = fb + (size_t)(y0 + y) * w + x0;
        for (int x = 0; x < rw; x++) {
            uint32_t c = row[x];
            row[x] = 0xFF000000u | (((c & 0xFF00FFu) * 90 >> 8) & 0xFF00FFu)
                                 | (((c & 0x00FF00u) * 90 >> 8) & 0x00FF00u);
        }
    }
}

static int au_text(uint32_t *fb, int w, int h, int x, int y, int s, uint32_t c, const char *t)
{
    for (; *t; t++) {
        char ch = *t;
        if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
        const char *g = au_glyph(ch);
        if (g) {
            for (int r = 0; r < 5; r++)
                for (int col = 0; col < 3; col++)
                    if (g[r * 3 + col] == '1')
                        au_rect(fb, w, h, x + col * s, y + r * s, s, s, c);
        }
        x += 4 * s;
    }
    return x;
}

static void au_bar(uint32_t *fb, int w, int h, int x, int ybot, int bw, int bh,
                   int s, uint16_t v, uint16_t hold, uint32_t c)
{
    au_rect(fb, w, h, x, ybot - bh, bw, bh, 0xFF141418u);          /* well */
    int fh = (int)((long)v * bh / 1024);
    if (fh > bh) fh = bh;
    if (fh > 0) {
        /* two-tone fill: brighter cap on top */
        uint32_t cap = 0xFF000000u
            | ((((c >> 16) & 255) + ((255 - ((c >> 16) & 255)) >> 1)) << 16)
            | ((((c >>  8) & 255) + ((255 - ((c >>  8) & 255)) >> 1)) <<  8)
            | (( (c        & 255) + ((255 - ( c        & 255)) >> 1)));
        au_rect(fb, w, h, x, ybot - fh, bw, fh, c);
        au_rect(fb, w, h, x, ybot - fh, bw, s > 1 ? s : 1, cap);
    }
    int hh = (int)((long)hold * bh / 1024);
    if (hh > 0 && hh <= bh)
        au_rect(fb, w, h, x, ybot - hh - (s > 1 ? s / 2 : 0), bw, s > 1 ? s / 2 : 1, 0xFFFFFFFFu);
}

void jd_audio_meter_draw(uint32_t *fb, int w, int h)
{
    if (!au_meter || !fb || w < 160 || h < 120) return;
    int s = h / 320; if (s < 1) s = 1; if (s > 6) s = 6;   /* 3 @ 960, 6 @ 2160 */
    int bw = 10 * s, gap = 4 * s, bh = 64 * s, pad = 6 * s;
    int panel_w = pad + 4 * (bw + gap) + 20 * s + pad;
    int panel_h = pad + 5 * s + 3 * s + 5 * s + 4 * s + bh + 3 * s + 5 * s + pad;
    int x0 = 12 * s, y0 = h - 12 * s - panel_h;
    if (panel_w > w - 2 * x0) return;
    au_dim(fb, w, h, x0, y0, panel_w, panel_h);
    au_rect(fb, w, h, x0, y0, panel_w, s > 1 ? s / 2 : 1, 0xFF404048u);

    /* header line 1: source kind + state + bpm; line 2: the device name */
    char head[128], dev[128];
    const char *kind  = au_src == SRC_TAP ? "TAP" : au_src == SRC_MIC ? "MIC" : "OFF";
    const char *state = !au_on ? "NO INPUT" : (au_stale > 15 ? "IDLE" : (g_audio.live ? "LIVE" : "QUIET"));
    if (g_audio.bpm_q8)
        snprintf(head, sizeof head, "%s %s %dBPM", kind, state, g_audio.bpm_q8 * 4 / 256);
    else
        snprintf(head, sizeof head, "%s %s", kind, state);
    snprintf(dev, sizeof dev, "%s", au_on ? au_srcname : "-");
    int maxch = (panel_w - 2 * pad) / (4 * s);       /* clip to the panel */
    if (maxch > 0 && maxch < (int)sizeof head) { head[maxch] = 0; dev[maxch] = 0; }
    uint32_t hc = g_audio.live ? 0xFFE8E8F0u : 0xFF9090A0u;
    au_text(fb, w, h, x0 + pad, y0 + pad, s, hc, head);
    au_text(fb, w, h, x0 + pad, y0 + pad + 8 * s, s, 0xFF8C8C9Cu, dev);

    int ybot = y0 + pad + 5 * s + 3 * s + 5 * s + 4 * s + bh;
    int x = x0 + pad;
    static const uint32_t COL[4] = { 0xFFFF6A2Au, 0xFFB8E03Cu, 0xFF3CC8FFu, 0xFFF0F0F0u };
    static const char *LBL[4] = { "B", "M", "T", "L" };
    uint16_t v[4] = { g_audio.bass, g_audio.mid, g_audio.treble, g_audio.level };
    for (int i = 0; i < 4; i++) {
        au_bar(fb, w, h, x, ybot, bw, bh, s, v[i], au_peak_hold[i], COL[i]);
        au_text(fb, w, h, x + (bw - 3 * s) / 2, ybot + 3 * s, s, 0xFFC0C0C8u, LBL[i]);
        x += bw + gap;
    }
    /* beat dot: a disc that flares to full on the onset and fades */
    {
        int r = 6 * s, cx = x + 8 * s, cy = ybot - bh / 2;
        uint32_t b = g_audio.beat;                    /* 0..1024 */
        uint32_t rr = 60 + (b * 195 >> 10), gg = 20 + (b * 60 >> 10), bb = 80 + (b * 175 >> 10);
        uint32_t c = 0xFF000000u | (rr << 16) | (gg << 8) | bb;
        for (int dy = -r; dy <= r; dy++)
            for (int dx = -r; dx <= r; dx++) {
                int d2 = dx * dx + dy * dy;
                if (d2 > r * r) continue;
                int px = cx + dx, py = cy + dy;
                if (px < 0 || py < 0 || px >= w || py >= h) continue;
                fb[(size_t)py * w + px] = (d2 > (r - s) * (r - s)) ? 0xFF505058u : c;
            }
        au_text(fb, w, h, cx - 3 * s / 2 - 2 * s, ybot + 3 * s, s, 0xFFC0C0C8u, "BEAT");
    }
}

/* ---- ABOUT (key A) -----------------------------------------------------
 * Where this came from, where the library lives, and the key map.  It lives
 * in this file because the 3x5 bitmap font and the panel primitives do; it
 * has nothing to do with audio beyond sharing them. Centred, so it reads as
 * a card rather than a debug readout. */
void jd_about_draw(uint32_t *fb, int w, int h)
{
    if (!au_about || !fb || w < 200 || h < 160) return;
    int s = h / 300; if (s < 1) s = 1; if (s > 7) s = 7;
    static const char *L[] = {
        "JELLYDAZZLE " JD_VERSION,
        "AN HOMAGE TO DAZZLE.EXE",
        "BY JAMES R. SHIFLETT, 1990",
        "",
        "DAZZLE.JELIA.NYC",
        "DAZZLE.JELIA.NYC/LIBRARY",
        "DAZZLE.JELIA.NYC/TRIBUTE",
        "GITHUB.COM/LIBCSYS/JELLYDAZZLE",
        "",
        "F FULLSCREEN   M METER",
        "A ABOUT        ESC QUIT",
        "",
        "MIT LICENCE - JOHN ELIA",
    };
    const int n = (int)(sizeof L / sizeof L[0]);
    int maxlen = 0;
    for (int i = 0; i < n; i++) { int l = (int)strlen(L[i]); if (l > maxlen) maxlen = l; }
    int pad = 8 * s, lh = 8 * s;
    int panel_w = pad + maxlen * 4 * s + pad;
    int panel_h = pad + n * lh + pad;
    if (panel_w > w || panel_h > h) return;
    int x0 = (w - panel_w) / 2, y0 = (h - panel_h) / 2;
    au_dim(fb, w, h, x0, y0, panel_w, panel_h);
    au_dim(fb, w, h, x0, y0, panel_w, panel_h);           /* twice: darker card */
    au_rect(fb, w, h, x0, y0, panel_w, s > 1 ? s / 2 : 1, 0xFF3A4652u);
    au_rect(fb, w, h, x0, y0 + panel_h - (s > 1 ? s / 2 : 1), panel_w,
            s > 1 ? s / 2 : 1, 0xFF3A4652u);
    for (int i = 0; i < n; i++) {
        if (!L[i][0]) continue;
        uint32_t c = (i == 0) ? 0xFF22D3EEu               /* title */
                   : (i >= 4 && i <= 7) ? 0xFFE9B65Au     /* the sites */
                   : 0xFFA8B6C4u;
        au_text(fb, w, h, x0 + pad, y0 + pad + i * lh, s, c, L[i]);
    }

    /* NOW PLAYING — name what is actually on screen, bottom to top, so a
     * viewer can report "I keep seeing 083 patch quilt" instead of
     * describing it.  This is the readout that turns an impression into a
     * fact, and it is the whole reason the registry carries names. */
    {
        jd_nowplaying np[8];
        int k = jd_now_playing(np, 8);
        int ly = y0 + panel_h + 6 * s;
        au_dim(fb, w, h, x0, ly - 3 * s, panel_w, (k + 1) * lh + 6 * s);
        au_text(fb, w, h, x0 + pad, ly, s, 0xFF7F8C99u, "NOW PLAYING");
        for (int i = 0; i < k; i++) {
            char row[96];
            snprintf(row, sizeof row, "%-6s %s",
                     jd_role_name(np[i].role), jd_routine_name(np[i].routine));
            int maxc2 = (panel_w - 2 * pad) / (4 * s);
            if (maxc2 > 0 && maxc2 < (int)sizeof row) row[maxc2] = 0;
            au_text(fb, w, h, x0 + pad, ly + (i + 1) * lh, s, 0xFFC8D4DEu, row);
        }
    }
}

/* ---- first-run status --------------------------------------------------
 * The engine measures every routine once on a new install — with 603 patterns
 * that is roughly half a minute. Without a word on screen it reads as "the
 * app is broken", so say what is happening and how far along it is. Drawn
 * bottom-left, small, and it disappears the moment measuring finishes. */
void jd_status_draw(uint32_t *fb, int w, int h, int pct, int secs)
{
    if (!fb || w < 240 || h < 180) return;
    if (pct < 0) pct = 0; if (pct > 100) pct = 100;
    int s = h / 200; if (s < 2) s = 2; if (s > 9) s = 9;   /* big: this matters */
    static const char *L1 = "FIRST RUN";
    static const char *L2 = "BUILDING THE PATTERN DATABASE";
    static const char *L3 = "THIS HAPPENS ONCE";
    char L4[64], L5[64];
    snprintf(L4, sizeof L4, "%d%%   %d:%02d ELAPSED", pct, secs / 60, secs % 60);
    snprintf(L5, sizeof L5, "PRESS SPACE TO CONTINUE ANYWAY");
    int w1 = (int)strlen(L1) * 4 * s, w2 = (int)strlen(L2) * 4 * s;
    int w3 = (int)strlen(L3) * 4 * s, w4 = (int)strlen(L4) * 4 * s;
    int w5 = (int)strlen(L5) * 4 * s;
    int tw = w2; if (w3 > tw) tw = w3; if (w4 > tw) tw = w4; if (w5 > tw) tw = w5;
    int pad = 10 * s, lh = 9 * s;
    int panel_w = tw + 2 * pad, panel_h = pad + lh * 5 + 8 * s + pad;
    if (panel_w > w) { s = s > 2 ? s - 1 : 2; }            /* shrink once if tight */
    int x0 = (w - panel_w) / 2, y0 = (h - panel_h) / 2;
    if (x0 < 0) x0 = 0;
    au_dim(fb, w, h, x0, y0, panel_w, panel_h);
    au_dim(fb, w, h, x0, y0, panel_w, panel_h);            /* twice: readable card */
    au_rect(fb, w, h, x0, y0, panel_w, s > 2 ? s / 2 : 1, 0xFF3A4652u);
    au_rect(fb, w, h, x0, y0 + panel_h - (s > 2 ? s / 2 : 1), panel_w,
            s > 2 ? s / 2 : 1, 0xFF3A4652u);
    au_text(fb, w, h, x0 + (panel_w - w1) / 2, y0 + pad,            s, 0xFFE9B65Au, L1);
    au_text(fb, w, h, x0 + (panel_w - w2) / 2, y0 + pad + lh,       s, 0xFFE8EEF4u, L2);
    au_text(fb, w, h, x0 + (panel_w - w3) / 2, y0 + pad + lh * 2,   s, 0xFF8FA2B4u, L3);
    au_text(fb, w, h, x0 + (panel_w - w4) / 2, y0 + pad + lh * 3,   s, 0xFF22D3EEu, L4);
    au_text(fb, w, h, x0 + (panel_w - w5) / 2, y0 + pad + lh * 4,   s, 0xFF6E7F90u, L5);
    /* progress bar, full width of the card */
    int bx = x0 + pad, by = y0 + pad + lh * 5 + 2 * s, bw = panel_w - 2 * pad;
    au_rect(fb, w, h, bx, by, bw, 2 * s, 0xFF243040u);
    au_rect(fb, w, h, bx, by, bw * pct / 100, 2 * s, 0xFF22D3EEu);
}
