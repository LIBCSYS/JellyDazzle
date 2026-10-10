/* ============================================================
 * ios_host.m — iPhone/iPad side of the INT 10h shim (3.5.1)
 *
 * Two jobs, both small:
 *   1. Power state for main.c's Lite/Full switch.
 *   2. Stand in for systap.m, the Mac's Core Audio process tap. iOS lets no
 *      app hear another app's output, so on a phone the "tap" never opens and
 *      listen.c falls straight through to the microphone, which hears the
 *      music coming out of the speaker. That is the whole iOS audio story.
 *
 * Built with -fobjc-arc (tools/build_ios.sh), unlike the Mac .m files.
 * ============================================================ */
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

/* ---- 1. power ------------------------------------------------------------ */

/* Lite when ANY of these is true:
 *   - unplugged             (batteryState Unplugged; Unknown = simulator, treated as plugged)
 *   - Low Power Mode is on  (the user has asked every app to save battery: honour it,
 *                            even on the charger)
 *   - the device is hot     (thermalState Serious or Critical: iOS is about to throttle
 *                            us anyway; doing less now keeps the frame rate steady)
 * Called every ~2 s from the frame loop: three property reads, no allocation. */
int jd_power_lite(void)
{
    static int armed = 0;
    UIDevice *d = [UIDevice currentDevice];
    if (!armed) { d.batteryMonitoringEnabled = YES; armed = 1; }   /* batteryState is Unknown until this */

    NSProcessInfo *pi = [NSProcessInfo processInfo];
    if (pi.lowPowerModeEnabled) return 1;
    NSProcessInfoThermalState t = pi.thermalState;
    if (t == NSProcessInfoThermalStateSerious || t == NSProcessInfoThermalStateCritical) return 1;

    /* JD_POWER=lite|full forces a mode — for the simulator, which always reports Unknown */
    const char *e = getenv("JD_POWER");
    if (e && !strcmp(e, "lite")) return 1;
    if (e && !strcmp(e, "full")) return 0;

    return d.batteryState == UIDeviceBatteryStateUnplugged;
}

/* 3.5.3: what the HUD shows about power. batt = 0..100 or -1 (simulator). */
void jd_power_info(int *batt, int *plugged, int *lowpower, int *thermal)
{
    UIDevice *d = [UIDevice currentDevice];
    float lv = d.batteryLevel;
    if (batt) *batt = lv < 0 ? -1 : (int)(lv * 100.0f + 0.5f);
    if (plugged) *plugged = d.batteryState == UIDeviceBatteryStateCharging || d.batteryState == UIDeviceBatteryStateFull;
    NSProcessInfo *pi = [NSProcessInfo processInfo];
    if (lowpower) *lowpower = pi.lowPowerModeEnabled;
    if (thermal) *thermal = (int)pi.thermalState;      /* 0 nominal 1 fair 2 serious 3 critical */
}

/* ---- 1b. microphone permission, asked WITHOUT blocking --------------------
 * Opening the mic before permission exists can stall the AudioQueue start
 * inside Apple's audio server (seen in the simulator 2026-10-08: SDL_main stuck
 * in au_open_mic -> AudioQueueStart, black screen forever). So the picture
 * starts first; iOS asks; main.c opens the mic only once this says granted.
 * Denied is a supported way to run: the engine dances on its own clocks. */
static volatile int g_mic = 0;            /* 0 asking, 1 granted, 2 denied */
void jd_mic_request(void)
{
    void (^done)(BOOL) = ^(BOOL ok) { g_mic = ok ? 1 : 2; };
    if (@available(iOS 17.0, *))
        [AVAudioApplication requestRecordPermissionWithCompletionHandler:done];
    else
        [[AVAudioSession sharedInstance] requestRecordPermission:done];
}
int jd_mic_state(void) { return g_mic; }

/* ---- 2. systap.m stand-ins ----------------------------------------------- */

typedef void (*jd_tap_push_fn)(const float *samples, int n, int stride);

int jd_systap_open(jd_tap_push_fn push, int *rate_out, char *name, int namelen)
{
    (void)push; (void)rate_out; (void)name; (void)namelen;
    return 0;                               /* never: listen.c uses the mic */
}
void jd_systap_close(void) {}
int  jd_systap_changed(void) { return 0; }
const char *jd_systap_error(void) { return "no system audio tap on iOS (microphone only)"; }
