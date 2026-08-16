/* systap.m — JellyDazzleAudio: capture what the Mac is PLAYING.
 *
 * A Core Audio *process tap* (macOS 14.2+, public API, no kernel extension,
 * no BlackHole, nothing to install) mixes the output of every process into a
 * private mono stream.  We hang that tap on a private aggregate device
 * clocked by the default output device and pull samples with an IOProc.
 *
 * What this means for the client: JellyDazzle hears Spotify / YouTube / any
 * app exactly as it is mixed, BEFORE the volume knob and regardless of
 * whether it is going to the speakers, headphones or Bluetooth — the room
 * microphone never enters into it.
 *
 * Facts measured on macOS 26.6 (2026-08-15) with a plain CLI binary:
 *   - AudioHardwareCreateProcessTap returned 0 with no permission prompt.
 *     (Since 14.4 Apple gates this behind Privacy & Security > Screen &
 *     System Audio Recording > "System Audio Recording Only".  A bundled
 *     .app should carry NSAudioCaptureUsageDescription in its Info.plist
 *     so the prompt can appear; if it is denied the tap yields silence and
 *     listen.c falls back to the microphone.)
 *   - Format delivered: 48000 Hz, 1 channel, Float32.
 *   - The IOProc only runs while the output device is actually running:
 *     with a Bluetooth headset and nothing playing, callback count freezes.
 *     listen.c treats a frozen callback counter as silence.
 *   - When the default output device changes (headphones on/off) the
 *     aggregate keeps clocking from the OLD device.  We watch the property
 *     and let listen.c rebuild the tap.
 *
 * Objective-C is required only because CATapDescription is an ObjC class;
 * everything else is C.  Nothing here touches the visuals.
 */
#import <Foundation/Foundation.h>
#import <CoreAudio/CoreAudio.h>
#import <AvailabilityMacros.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(MAC_OS_VERSION_14_2) || defined(__MAC_14_2)
#import <CoreAudio/AudioHardwareTapping.h>
#import <CoreAudio/CATapDescription.h>
#define JD_HAVE_TAP 1
#else
#define JD_HAVE_TAP 0
#endif

typedef void (*jd_tap_push_fn)(const float *samples, int n, int stride);

static jd_tap_push_fn  st_push;
static AudioObjectID   st_tap  = kAudioObjectUnknown;
static AudioObjectID   st_agg  = kAudioObjectUnknown;
static AudioDeviceIOProcID st_proc;
static int             st_running;
static int             st_channels = 1;
static volatile int    st_changed;         /* default output device moved */
static int             st_listening;
static char            st_err[96];        /* last failure, for the log      */

const char *jd_systap_error(void) { return st_err; }
#define ST_FAIL(stage, code) snprintf(st_err, sizeof st_err, "%s failed (OSStatus %d)", stage, (int)(code))

#if JD_HAVE_TAP
static OSStatus st_ioproc(AudioObjectID dev, const AudioTimeStamp *now,
                          const AudioBufferList *in, const AudioTimeStamp *inT,
                          AudioBufferList *out, const AudioTimeStamp *outT, void *ud)
{
    (void)dev; (void)now; (void)inT; (void)out; (void)outT; (void)ud;
    if (!st_push || !in) return noErr;
    for (UInt32 b = 0; b < in->mNumberBuffers; b++) {
        const AudioBuffer *ab = &in->mBuffers[b];
        int ch = (int)ab->mNumberChannels; if (ch < 1) ch = 1;
        int n  = (int)(ab->mDataByteSize / sizeof(float)) / ch;
        if (n > 0 && ab->mData) st_push((const float *)ab->mData, n, ch);
    }
    return noErr;
}

static OSStatus st_defout_listener(AudioObjectID obj, UInt32 n,
                                   const AudioObjectPropertyAddress *addr, void *ud)
{
    (void)obj; (void)n; (void)addr; (void)ud;
    st_changed = 1;
    return noErr;
}

static const AudioObjectPropertyAddress ST_DEFOUT = {
    kAudioHardwarePropertyDefaultOutputDevice,
    kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
#endif

/* Open the system tap.  Returns 1 and fills rate/name on success. */
int jd_systap_open(jd_tap_push_fn push, int *rate_out, char *name, int namelen)
{
#if !JD_HAVE_TAP
    (void)push; (void)rate_out; (void)name; (void)namelen;
    return 0;
#else
    if (st_running) return 1;
    if (@available(macOS 14.2, *)) {} else { snprintf(st_err, sizeof st_err, "needs macOS 14.2+"); return 0; }
    st_err[0] = 0;
    @autoreleasepool {
        st_push = push;
        OSStatus st;

        /* 1. tap: every process, mixed to mono, private to us, unmuted
         *    (the client keeps hearing his music). */
        CATapDescription *d = [[CATapDescription alloc] initMonoGlobalTapButExcludeProcesses:@[]];
        d.name = @"JellyDazzle listener";
        d.privateTap = YES;
        d.muteBehavior = CATapUnmuted;
        st = AudioHardwareCreateProcessTap(d, &st_tap);
        if (st != noErr) { ST_FAIL("AudioHardwareCreateProcessTap", st); st_tap = kAudioObjectUnknown; return 0; }

        AudioObjectPropertyAddress pa = { kAudioTapPropertyFormat,
            kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
        AudioStreamBasicDescription asbd; UInt32 sz = sizeof asbd;
        memset(&asbd, 0, sizeof asbd);
        if (AudioObjectGetPropertyData(st_tap, &pa, 0, NULL, &sz, &asbd) == noErr) {
            if (rate_out && asbd.mSampleRate > 1000) *rate_out = (int)asbd.mSampleRate;
            st_channels = asbd.mChannelsPerFrame ? (int)asbd.mChannelsPerFrame : 1;
        }
        pa.mSelector = kAudioTapPropertyUID;
        CFStringRef tapUID = NULL; sz = sizeof tapUID;
        st = AudioObjectGetPropertyData(st_tap, &pa, 0, NULL, &sz, &tapUID);
        if (st != noErr || !tapUID) { ST_FAIL("kAudioTapPropertyUID", st); AudioHardwareDestroyProcessTap(st_tap); st_tap = 0; return 0; }

        /* 2. the default output device is the aggregate's clock */
        AudioObjectID outdev = 0; sz = sizeof outdev;
        AudioObjectGetPropertyData(kAudioObjectSystemObject, &ST_DEFOUT, 0, NULL, &sz, &outdev);
        CFStringRef outUID = NULL; sz = sizeof outUID;
        AudioObjectPropertyAddress ua = { kAudioDevicePropertyDeviceUID,
            kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
        if (outdev) AudioObjectGetPropertyData(outdev, &ua, 0, NULL, &sz, &outUID);
        if (name && namelen > 0) {
            CFStringRef nm = NULL; sz = sizeof nm;
            AudioObjectPropertyAddress na = { kAudioObjectPropertyName,
                kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
            if (outdev) AudioObjectGetPropertyData(outdev, &na, 0, NULL, &sz, &nm);
            snprintf(name, (size_t)namelen, "%s",
                     nm ? [(__bridge NSString *)nm UTF8String] : "system output");
            if (nm) CFRelease(nm);
        }

        /* UID must be unique PER PROCESS: a run that dies by signal leaves
         * its private aggregate registered in coreaudiod for a while, and a
         * second CreateAggregateDevice with the same UID fails with 'nope'
         * (kAudioHardwareIllegalOperationError 1852797029) — measured. */
        NSString *aggUID = [NSString stringWithFormat:@"nyc.jelia.jellydazzle.listener.%d.%u",
                            (int)getpid(), (unsigned)arc4random()];
        NSMutableDictionary *desc = [@{
            @kAudioAggregateDeviceNameKey:         @"JellyDazzle listener",
            @kAudioAggregateDeviceUIDKey:          aggUID,
            @kAudioAggregateDeviceIsPrivateKey:    @YES,
            @kAudioAggregateDeviceIsStackedKey:    @NO,
            @kAudioAggregateDeviceTapAutoStartKey: @YES,
            @kAudioAggregateDeviceTapListKey: @[ @{
                @kAudioSubTapUIDKey: (__bridge NSString *)tapUID,
                @kAudioSubTapDriftCompensationKey: @YES } ],
        } mutableCopy];
        if (outUID)
            desc[@kAudioAggregateDeviceSubDeviceListKey] =
                @[ @{ @kAudioSubDeviceUIDKey: (__bridge NSString *)outUID } ];
        st = AudioHardwareCreateAggregateDevice((__bridge CFDictionaryRef)desc, &st_agg);
        CFRelease(tapUID);
        if (outUID) CFRelease(outUID);
        if (st != noErr) {
            ST_FAIL("AudioHardwareCreateAggregateDevice", st);
            AudioHardwareDestroyProcessTap(st_tap); st_tap = 0; st_agg = 0; return 0;
        }

        /* 3. pull */
        st = AudioDeviceCreateIOProcID(st_agg, st_ioproc, NULL, &st_proc);
        if (st == noErr) st = AudioDeviceStart(st_agg, st_proc);
        if (st != noErr) {
            ST_FAIL("AudioDeviceCreateIOProcID/Start", st);
            if (st_proc) AudioDeviceDestroyIOProcID(st_agg, st_proc);
            AudioHardwareDestroyAggregateDevice(st_agg);
            AudioHardwareDestroyProcessTap(st_tap);
            st_agg = 0; st_tap = 0; st_proc = NULL; return 0;
        }
        if (!st_listening) {
            AudioObjectAddPropertyListener(kAudioObjectSystemObject, &ST_DEFOUT,
                                           st_defout_listener, NULL);
            st_listening = 1;
        }
        st_changed = 0;
        st_running = 1;
        return 1;
    }
#endif
}

void jd_systap_close(void)
{
#if JD_HAVE_TAP
    if (!st_running) return;
    if (st_proc) { AudioDeviceStop(st_agg, st_proc); AudioDeviceDestroyIOProcID(st_agg, st_proc); }
    if (st_agg)  AudioHardwareDestroyAggregateDevice(st_agg);
    if (st_tap)  AudioHardwareDestroyProcessTap(st_tap);
    st_proc = NULL; st_agg = 0; st_tap = 0; st_running = 0;
#endif
}

/* 1 once after the default output device changed (headphones plugged /
 * unplugged, Bluetooth connect); the caller closes and reopens. */
int jd_systap_changed(void)
{
    int c = st_changed; st_changed = 0; return c;
}
