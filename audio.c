/* audio.c — JellyDazzleAudio: microphone -> dance.
 *
 * Captures whatever the Mac can hear (music from speakers, a guitar, a room),
 * turns it into a handful of smooth numbers, and hands them to the engine:
 *
 *     bass / mid / treble   band energies, 0..1024, attack-fast release-slow
 *     level                 overall loudness, 0..1024
 *     beat                  a pulse that spikes to 1024 on an onset and decays
 *     bpm_q8                running tempo estimate (Q8), 0 if unsure
 *
 * Design rules, learned from this project's whole history:
 *   - NOTHING STROBES.  Every value is enveloped (fast attack, slow release)
 *     and the visuals read these as targets, not as instant jumps.
 *   - SILENCE IS FINE.  With no signal the values decay to zero and the
 *     engine's own clocks carry the show exactly as before.
 *   - CHEAP.  A 512-point real FFT once per frame is ~30 us; the capture
 *     callback only copies into a ring buffer.
 */
#include <SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "jellydazzle.h"

#define AU_RATE   44100
#define AU_N      512                  /* FFT size (11.6 ms @ 44.1k)        */
#define AU_RING   (AU_N * 8)

jd_audio g_audio;                      /* the public, read-only-ish state   */

static float    au_ring[AU_RING];
static volatile int au_w;              /* ring write cursor                 */
static SDL_AudioDeviceID au_dev;
static int      au_on;

static float    au_win[AU_N];          /* Hann window                       */
static float    au_prev[AU_N / 2];     /* previous magnitudes (for flux)    */
static float    au_flux_avg;           /* adaptive onset threshold          */
static int      au_since_beat;
static int      au_beat_gap[8], au_gap_i;

/* ---- capture callback: copy in, do no work ---- */
static volatile int au_cbs;        /* callbacks seen        */
static volatile float au_peak;     /* loudest sample seen   */

static void au_cb(void *ud, Uint8 *stream, int len)
{
    (void)ud;
    au_cbs++;
    const float *in = (const float *)stream;
    int n = len / (int)sizeof(float);
    int w = au_w;
    for (int i = 0; i < n; i++) {
        float v = in[i] < 0 ? -in[i] : in[i];
        if (v > au_peak) au_peak = v;
        au_ring[w] = in[i];
        w = (w + 1) % AU_RING;
    }
    au_w = w;
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

int jd_audio_init(void)
{
    if (au_on) return 1;
    /* main.c starts SDL with VIDEO only; capture needs the audio subsystem
     * brought up explicitly or SDL_OpenAudioDevice fails with
     * "Audio subsystem is not initialized". */
    if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        SDL_Log("JellyDazzleAudio: audio subsystem unavailable (%s)", SDL_GetError());
        return 0;
    }
    for (int i = 0; i < AU_N; i++)
        au_win[i] = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * i / (AU_N - 1));

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq     = AU_RATE;
    want.format   = AUDIO_F32SYS;
    want.channels = 1;
    want.samples  = 1024;
    want.callback = au_cb;

    /* Pick a REAL microphone.  The system default input can be a virtual
     * device (Teams, Zoom, a disconnected webcam) that delivers digital
     * silence forever — which is exactly what we measured: the callback
     * fired 800 times with every sample 0.0000.  Prefer the built-in mic,
     * then anything that is not a known virtual driver, and only then fall
     * back to the default.  JD_AUDIO_DEV=<n> forces a specific index. */
    const char *want_name = NULL;
    int ndev = SDL_GetNumAudioDevices(1);
    const char *force = SDL_getenv("JD_AUDIO_DEV");
    if (force) {
        int fi = SDL_atoi(force);
        if (fi >= 0 && fi < ndev) want_name = SDL_GetAudioDeviceName(fi, 1);
    }
    if (!want_name) {
        for (int i = 0; i < ndev && !want_name; i++) {
            const char *n = SDL_GetAudioDeviceName(i, 1);
            if (n && SDL_strstr(n, "MacBook") && SDL_strstr(n, "Micro"))
                want_name = n;
        }
    }
    if (!want_name) {
        static const char *virt[] = { "Teams", "Zoom", "BlackHole", "Loopback",
                                      "Aggregate", "Soundflower", NULL };
        for (int i = 0; i < ndev && !want_name; i++) {
            const char *n = SDL_GetAudioDeviceName(i, 1);
            int bad = 0;
            for (int v = 0; virt[v] && n; v++) if (SDL_strstr(n, virt[v])) bad = 1;
            if (n && !bad) want_name = n;
        }
    }
    au_dev = SDL_OpenAudioDevice(want_name, 1, &want, &have, 0);
    if (!au_dev) au_dev = SDL_OpenAudioDevice(NULL, 1, &want, &have, 0);
    {
        FILE *lf = fopen("/tmp/jd_audio_meter.log", "a");
        if (lf) { fprintf(lf, "device=\"%s\" of %d\n",
                          want_name ? want_name : "(system default)", ndev); fclose(lf); }
    }
    if (!au_dev) {
        SDL_Log("JellyDazzleAudio: no microphone (%s) — running on internal "
                "clocks", SDL_GetError());
        return 0;
    }
    {
        FILE *lf = fopen("/tmp/jd_audio_meter.log", "a");
        if (lf) {
            fprintf(lf, "opened dev=%d freq=%d ch=%d fmt=0x%x samples=%d\n",
                    (int)au_dev, have.freq, have.channels, have.format, have.samples);
            fclose(lf);
        }
    }
    SDL_PauseAudioDevice(au_dev, 0);
    au_on = 1;
    return 1;
}

void jd_audio_close(void)
{
    if (au_on) { SDL_CloseAudioDevice(au_dev); au_on = 0; }
}

/* envelope: fast attack, slow release, so nothing flickers */
static uint16_t env(uint16_t cur, float target_f)
{
    int t = (int)(target_f > 1.0f ? 1024.0f : target_f * 1024.0f);
    if (t < 0) t = 0;
    return (uint16_t)(t > cur ? cur + ((t - cur) >> 1)     /* attack ~2 frames */
                              : cur - ((cur - t) >> 4));   /* release ~16      */
}

void jd_audio_tick(void)
{
    if (!au_on) {                       /* decay to nothing, stay valid */
        g_audio.level = (uint16_t)(g_audio.level * 15 / 16);
        g_audio.bass  = (uint16_t)(g_audio.bass  * 15 / 16);
        g_audio.mid   = (uint16_t)(g_audio.mid   * 15 / 16);
        g_audio.treble= (uint16_t)(g_audio.treble* 15 / 16);
        g_audio.beat  = (uint16_t)(g_audio.beat  * 7 / 8);
        g_audio.live  = 0;
        return;
    }

    /* newest AU_N samples out of the ring */
    static float re[AU_N], im[AU_N];
    int w = au_w;
    for (int i = 0; i < AU_N; i++) {
        int idx = (w - AU_N + i + AU_RING) % AU_RING;
        re[i] = au_ring[idx] * au_win[i];
        im[i] = 0.0f;
    }

    float rms = 0.0f;
    for (int i = 0; i < AU_N; i++) rms += re[i] * re[i];
    rms = sqrtf(rms / AU_N);

    au_fft(re, im, AU_N);

    /* band edges in bins: 44100/512 = 86 Hz per bin */
    float b_bass = 0, b_mid = 0, b_tre = 0, flux = 0;
    for (int k = 1; k < AU_N / 2; k++) {
        float m = sqrtf(re[k] * re[k] + im[k] * im[k]);
        float d = m - au_prev[k];
        if (d > 0) flux += d;                       /* spectral flux: onsets */
        au_prev[k] = m;
        if      (k <= 3)  b_bass += m;              /*   ~86-260 Hz */
        else if (k <= 24) b_mid  += m;              /*  ~350-2 kHz  */
        else              b_tre  += m;              /*   2k-22 kHz  */
    }

    /* normalise: these constants are tuned for typical room levels and the
     * envelope hides the rest */
    g_audio.level  = env(g_audio.level,  rms * 45.0f);   /* measured: room speech was 21/1024 at 6x */
    g_audio.bass   = env(g_audio.bass,   b_bass * 0.28f);
    g_audio.mid    = env(g_audio.mid,    b_mid  * 0.16f);
    g_audio.treble = env(g_audio.treble, b_tre  * 0.16f);
    g_audio.live   = (uint16_t)(g_audio.level > 55);   /* engages on real sound, ignores hiss */

    /* ---- onset detection: flux above an adaptive mean, with a refractory
     * gap so a sustained note is not a drum roll ---- */
    au_flux_avg = au_flux_avg * 0.92f + flux * 0.08f;
    au_since_beat++;
    if (flux > au_flux_avg * 1.55f && au_since_beat > 8 && g_audio.level > 90) {
        g_audio.beat = 1024;
        au_beat_gap[au_gap_i++ & 7] = au_since_beat;
        au_since_beat = 0;
        /* median-ish tempo from the last 8 gaps (frames -> BPM, 60 fps) */
        int s = 0, n = 0;
        for (int i = 0; i < 8; i++) if (au_beat_gap[i]) { s += au_beat_gap[i]; n++; }
        if (n >= 4) {
            float gap = (float)s / n;               /* frames between beats */
            float bpm = 3600.0f / gap;              /* 60 fps * 60 s        */
            if (bpm > 60.0f && bpm < 190.0f)
                g_audio.bpm_q8 = (uint16_t)(bpm * 256.0f / 4.0f); /* Q8/4 */
        }
    } else {
        g_audio.beat = (uint16_t)(g_audio.beat * 7 / 8);   /* ~0.2 s decay */
    }

    /* JD_AUDIO_DEBUG=1 prints the meter ~4x a second, for tuning by ear */
    {
        static int dbg = -1, tick;
        if (dbg < 0) dbg = SDL_getenv("JD_AUDIO_DEBUG") ? 1 : 0;
        if (dbg && (++tick % 15) == 0) {
            FILE *lf = fopen("/tmp/jd_audio_meter.log", "a");
            if (lf) {
                fprintf(lf, "cb=%d peak=%.4f live=%d level=%4d bass=%4d mid=%4d treble=%4d beat=%4d bpm=%d\n",
                        au_cbs, au_peak,
                        g_audio.live, g_audio.level, g_audio.bass, g_audio.mid,
                        g_audio.treble, g_audio.beat, g_audio.bpm_q8 * 4 / 256);
                fclose(lf);
            }
            SDL_Log("live=%d level=%4d bass=%4d mid=%4d treble=%4d beat=%4d bpm=%d",
                    g_audio.live, g_audio.level, g_audio.bass, g_audio.mid,
                    g_audio.treble, g_audio.beat, g_audio.bpm_q8 * 4 / 256);
        }
    }
}
