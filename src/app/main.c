/* ============================================================
 * main.c — the "INT 10h" shim
 *
 * In DOS, dazzle.exe called INT 10h to enter mode 13h and got a
 * flat framebuffer at A000:0000. macOS won't hand raw video
 * memory to a process, so this tiny C file plays that role:
 * open a window, own a flat pixel buffer, and blit it to the
 * screen every frame. ALL drawing happens in draw.s.
 * ============================================================ */

#include <SDL.h>
#include <TargetConditionals.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#if TARGET_OS_IPHONE
/* src/app/ios_host.m — the phone/tablet side: power state, no menu bar */
extern int  jd_power_lite(void);      /* 1 = on battery, Low Power Mode, or hot */
extern void jd_mic_request(void);     /* async permission prompt */
extern int  jd_mic_state(void);       /* 0 asking, 1 granted, 2 denied */
extern void jd_power_info(int *batt, int *plugged, int *lowpower, int *thermal);
extern void jd_perf_get(double *ms, int *hot, int *live, int *cap, double *budk);
/* 3.5.9: the gate thresholds in compositor.c were tuned against a 16.7 ms Mac
 * frame. Lite draws every other refresh, so a Lite frame actually has 33 ms —
 * scaling them keeps the SAME proportion of the real budget instead of calling
 * a comfortable 14 ms frame "hot" and refusing upper layers that fit easily.
 * This function shipped in 3.5.3 and was never called, so every Lite frame
 * since has been judged against a budget twice as tight as the one it had. */
extern void jd_set_frame_budget(double k);
extern void jd_hud_lines(uint32_t *fb, int w, int h, const char *const *lines, int n);
static int hud_on = 0;                /* three-finger tap */
static int res_req = 0;               /* two-finger tap while the readout is up */
/* Resolution presets the phone can A/B through, per mode. Index 0 = the shipping default. */
static const long FULL_SET[] = { 2000000, 3300000, 1600000 };
static const long LITE_SET[] = { 1400000, 1000000, 2000000 };
static int full_i = 0, lite_i = 0;
#else
extern void jd_menu_install(void);    /* src/app/menu_mac.m */
#endif
extern void jd_about_toggle(void);    /* src/audio/listen.c */
extern int  jd_about_is_on(void);
extern void jd_ui_skip(void);         /* src/audio/listen.c: dismiss the first-run notice */
extern void jd_set_slot_cap(int n);   /* src/engine/compositor.c */
extern int  jd_req_palette, jd_req_shape;

#ifndef JD_VERSION
#define JD_VERSION "dev"   /* set by the Makefile from ./VERSION */
#endif

#define W 1280                       /* opening window size            */
#define H 960
/* Render at the window's REAL pixel size so full screen is sharp instead of
 * a stretched 1280x960 image.  Capped by area: past ~3.6 Mpx the frame cost
 * outruns 60 Hz on the heaviest layer stacks, so beyond that we render at
 * the largest same-aspect size within budget and let the GPU do the last
 * (small, and therefore invisible) bit of scaling. */
#define MAX_PIX 8300000   /* measured: 103 fps at 3456x2160 (7.5 Mpx), so
                           * full Retina renders natively — no soft scaling */

/* 3.5.1 LITE mode (iPhone/iPad on battery). Every pixel is drawn by the CPU, so
 * the three levers on battery life are pixels, frames and layers:
 *   pixels  — render about 1.4 Mpx and let the GPU scale up (a phone screen is ~3 Mpx)
 *   frames  — draw every other display refresh: 30 fps, same tempo (frame += 2)
 *   layers  — all four since 3.5.4 (J: richer Lite); 3.5.1-3.5.3 held the spark layer back
 * FULL is the Mac experience: native resolution, 60 fps, all four layers. */
#define LITE_PIX   1400000
/* iOS Full: a phone screen is ~3.2 Mpx and renders native; a 13" iPad (5.7 Mpx)
 * measured 34 fps native on M5-class silicon, so it renders at 3.3 Mpx and the
 * GPU scales the last bit — the same trade the Mac makes past MAX_PIX. */
#define IOS_FULL_PIX 2000000   /* J 2026-10-08, after A/B on his iPhone 15 Pro Max: 2.0 Mpx.
                                * Native (3.2) ran HOT and dropped to ~2.7 live layers; 2.0
                                * holds ~3.6 at the same frame rate — the "pizazz" is layers. */
/* 2.0 would match the 33 ms frame exactly; 1.8 keeps a margin for thermal
 * throttling and the compositor's own work. Watch it on the three-finger HUD. */
#define LITE_BUDK  1.8
#define FULL_BUDK  1.0
#define LITE_SLOTS 4   /* J 2026-10-08: "richer" Lite — the spark layer back; 30 fps and
                        * 1.4 Mpx still hold it near half of Full's energy */
#define FULL_SLOTS 4

/* implemented in draw.s */
extern void jd_frame(uint32_t *fb, int width, int height, int frame);
extern int  jd_audio_init(void);
extern void jd_audio_tick(void);
extern void jd_audio_close(void);

static uint32_t *framebuffer;         /* our A000:0000, sized to the window */
static int fb_w, fb_h;
#if TARGET_OS_IPHONE
#define FULL_PIX IOS_FULL_PIX
#else
#define FULL_PIX MAX_PIX
#endif
static long pix_cap = FULL_PIX;       /* FULL_PIX in Full, LITE_PIX in Lite */
static long full_pix = FULL_PIX, lite_pix = LITE_PIX;   /* JD_FULLPIX / JD_LITEPIX override, for measuring */

/* choose the render size for a given drawable size */
static void fb_pick(int dw, int dh, int *rw, int *rh)
{
    if (dw < 64) dw = 64;
    if (dh < 64) dh = 64;
    double scale = 1.0;
    double area = (double)dw * dh;
    if (area > pix_cap) scale = sqrt(pix_cap / area);
    *rw = (int)(dw * scale) & ~1;
    *rh = (int)(dh * scale) & ~1;
}

/* Re-pick the render size (window resized, phone rotated, Lite <-> Full) and
 * swap buffers only if it changed. On failure the old pair stays in use. */
static void fb_refit(SDL_Renderer *ren, SDL_Texture **tex)
{
    int ndw = fb_w, ndh = fb_h, nw, nh;
    SDL_GetRendererOutputSize(ren, &ndw, &ndh);
    fb_pick(ndw, ndh, &nw, &nh);
    if (nw == fb_w && nh == fb_h) return;
    uint32_t *nb = (uint32_t *)calloc((size_t)nw * nh, 4);
    SDL_Texture *nt = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STREAMING, nw, nh);
    if (nb && nt) {
        free(framebuffer); SDL_DestroyTexture(*tex);
        framebuffer = nb; *tex = nt; fb_w = nw; fb_h = nh;
    } else { free(nb); if (nt) SDL_DestroyTexture(nt); }
}

/* 3.5.5 — the button bar (J: "small and out of the way", C | S | ? and X).
 * Bottom-right, ~52 pt squares whatever the screen, bright for 4 s after any
 * touch or mouse movement, then a 1 s fade to a faint ghost that stays put so
 * it can always be found. No X on iPhone/iPad: Apple's HIG says an iOS app
 * never quits itself (and review can treat it as a crash) — the home gesture
 * is how you leave. On the Mac X does what Esc does. */
extern void jd_ui_button(uint32_t *fb, int w, int h, int x, int y, int size, char label, int a);
/* No X on iPhone/iPad. 3.5.7 added one (J: "X exits the app"); J reversed it the
 * same evening once the review risk was clear: "no sense tempting it… swipe up
 * works just as well". Apple's HIG: iOS apps never quit themselves. */
#if TARGET_OS_IPHONE
static const char BTN[] = "CS?";
#else
static const char BTN[] = "CS?X";
#endif
static SDL_Window *g_win;
static Uint32 ui_touch;               /* last touch / mouse activity, ms */

static void bar_geom(int *bx, int *by, int *bs, int *gap)
{
    int ww = 1, wh = 1;
    SDL_GetWindowSize(g_win, &ww, &wh);                   /* points */
    double k = ww > 0 ? fb_w / (double)ww : 1.0;          /* fb pixels per point */
    int n = (int)(sizeof BTN - 1);
    *bs = (int)(52 * k); *gap = (int)(10 * k);
    int margin = (int)(16 * k);
#if TARGET_OS_IPHONE
    int bottom = (int)(40 * k);                           /* clear of the home indicator */
#else
    int bottom = margin;
#endif
    *bx = fb_w - margin - n * *bs - (n - 1) * *gap;
    *by = fb_h - bottom - *bs;
}

static int bar_hit(double fx, double fy)                  /* fb coords -> button index or -1 */
{
    int bx, by, bs, gap;
    bar_geom(&bx, &by, &bs, &gap);
    int pad = gap / 2;                                    /* forgiving edges */
    if (fy < by - pad || fy > by + bs + pad) return -1;
    for (int i = 0; BTN[i]; i++) {
        int x = bx + i * (bs + gap);
        if (fx >= x - pad && fx <= x + bs + pad) return i;
    }
    return -1;
}

static void bar_act(int i, int *running)
{
    switch (BTN[i]) {
    case 'C': jd_req_palette = 1; break;
    case 'S': jd_req_shape = 1; break;
    case '?': jd_about_toggle(); break;
    case 'X':
#if TARGET_OS_IPHONE
        /* SDL's iOS main does not exit when SDL_main returns, so leave directly —
         * after closing the mic so the audio session is released cleanly. */
        jd_audio_close();
        exit(0);
#else
        if (running) *running = 0;
#endif
        break;
    }
    jd_ui_skip();                                         /* any button also clears the first-run card */
}

static void bar_draw(void)
{
    static int nobar = -1;                 /* JD_NOBAR: clean App Store screenshots */
    if (nobar < 0) nobar = getenv("JD_NOBAR") ? 1 : 0;
    if (nobar) return;
    Uint32 t = SDL_GetTicks() - ui_touch;
    int a = t < 4000 ? 220 : t < 5000 ? 220 - (int)((t - 4000) * 140 / 1000) : 80;   /* faint = ~31%: J reads with low vision */
    int bx, by, bs, gap;
    bar_geom(&bx, &by, &bs, &gap);
    for (int i = 0; BTN[i]; i++)
        jd_ui_button(framebuffer, fb_w, fb_h, bx + i * (bs + gap), by, bs, BTN[i], a);
}

#if TARGET_OS_IPHONE
/* Touch = the keyboard's C, S and A. One finger tap: new colours. Two finger
 * tap: new shapes. Press and hold: the about card. A drag is none of these, so
 * a hand brushing the glass does nothing. Normalised coords (0..1). */
static struct { int down, maxf, moved, held; Uint32 t0; float x0, y0; } tch;

static void touch_event(const SDL_Event *e)
{
    if (e->type == SDL_FINGERDOWN) {
        ui_touch = SDL_GetTicks();
        if (tch.down == 0) {
            tch.maxf = 0; tch.moved = 0; tch.held = 0;
            tch.t0 = SDL_GetTicks(); tch.x0 = e->tfinger.x; tch.y0 = e->tfinger.y;
        }
        tch.down++;
        if (tch.down > tch.maxf) tch.maxf = tch.down;
    } else if (e->type == SDL_FINGERMOTION) {
        float dx = e->tfinger.x - tch.x0, dy = e->tfinger.y - tch.y0;
        if (tch.down == 1 && dx * dx + dy * dy > 0.0016f) tch.moved = 1;  /* 4% of the screen */
    } else if (e->type == SDL_FINGERUP) {
        if (tch.down > 0) tch.down--;
        if (tch.down == 0 && !tch.moved && !tch.held && SDL_GetTicks() - tch.t0 < 500) {
            jd_ui_skip();                         /* any tap also clears the first-run notice */
            if (tch.maxf >= 3) hud_on ^= 1;       /* performance readout */
            else if (tch.maxf == 2 && hud_on) res_req = 1;  /* readout up: cycle resolution (A/B on the phone) */
            else if (tch.maxf == 2) jd_req_shape = 1;  /* S */
            else {
                int b = bar_hit(tch.x0 * fb_w, tch.y0 * fb_h);
                if (b >= 0) bar_act(b, NULL);         /* a button */
                else jd_req_palette = 1;               /* C: anywhere else */
            }
        }
    }
}

/* Long press fires WHILE the finger is still down (that is what "hold" feels
 * like), so it is polled from the frame loop, not from an event. */
static void touch_poll(void)
{
    if (tch.down == 1 && tch.maxf == 1 && !tch.moved && !tch.held
        && SDL_GetTicks() - tch.t0 >= 700) {
        tch.held = 1;
        jd_about_toggle();                        /* A */
    }
}

/* Three-finger tap: what the phone is actually doing. Mode and why, real fps
 * (frames DRAWN per second, not display refreshes), the engine's own smoothed
 * ms per frame and whether it is thinning the stack ("HOT"), layers live vs
 * allowed, render size, battery and iOS's thermal state. */
static void hud_draw(int lite, double fps)
{
    double ms = 0, budk = 1; int hot = 0, live = 0, cap = 0;
    int batt = -1, plugged = 0, lowp = 0, therm = 0;
    jd_perf_get(&ms, &hot, &live, &cap, &budk);
    jd_power_info(&batt, &plugged, &lowp, &therm);
    static const char *TH[] = { "NOMINAL", "FAIR", "SERIOUS", "CRITICAL" };
    char l0[64], l1[64], l2[64], l3[64], l4[64];
    /* short lines on purpose: the panel is sized to its longest line, so every
     * character cut makes the text bigger (3.5.4) */
    (void)budk;
    snprintf(l0, sizeof l0, "%s %s", lite ? "LITE" : "FULL",
             lowp ? "LOW POWER" : therm >= 2 ? "HOT" : plugged ? "CHARGING" : batt < 0 ? "SIM" : "BATTERY");
    snprintf(l1, sizeof l1, "%.0f FPS %.0f MS%s", fps, ms, hot ? " HOT" : "");
    snprintf(l2, sizeof l2, "LAYERS %d/%d", live, cap);
    snprintf(l3, sizeof l3, "%.1f MPX  SET %d/3", fb_w * (double)fb_h / 1e6, (lite ? lite_i : full_i) + 1);
    if (batt >= 0) snprintf(l4, sizeof l4, "BATT %d%% %s", batt, TH[therm & 3]);
    else           snprintf(l4, sizeof l4, "HEAT %s", TH[therm & 3]);
    const char *L[] = { l0, l1, l2, l3, l4 };
    jd_hud_lines(framebuffer, fb_w, fb_h, L, 5);
}
#endif

#if TARGET_OS_IPHONE
static int audio_open_thread(void *arg) { (void)arg; jd_audio_init(); return 0; }
#endif

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
#if TARGET_OS_IPHONE
    /* Hints BEFORE init. "playandrecord" + our SDL patch (vendor/sdl2-ios) = the mic
     * listens while your music app keeps playing at full volume and quality. */
    SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "playandrecord");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight Portrait PortraitUpsideDown");
    SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "1");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");   /* touches stay touches */
#endif
    SDL_Init(SDL_INIT_VIDEO);
#if !TARGET_OS_IPHONE
    /* Install the native menu bar AFTER SDL_Init: SDL builds its own bar during
     * init and only when [NSApp mainMenu] is nil, so we append to it. Doing this
     * first would make SDL skip its activation-policy setup entirely. */
    jd_menu_install();
    Uint32 wflags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#else
    Uint32 wflags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI;
#endif

    SDL_Window *win = SDL_CreateWindow(
        "JellyDazzle v" JD_VERSION,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H, wflags);

    g_win = win;
    ui_touch = SDL_GetTicks();                        /* bar starts bright, so it is noticed */
    SDL_Renderer *ren = SDL_CreateRenderer(
        win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    int lite = 0;
#if TARGET_OS_IPHONE
    lite = jd_power_lite();
    { const char *e = getenv("JD_FULLPIX"); if (e) full_pix = atol(e); e = getenv("JD_LITEPIX"); if (e) lite_pix = atol(e); }
    pix_cap = full_pix;
    if (lite) { pix_cap = lite_pix; jd_set_slot_cap(LITE_SLOTS); }
    jd_set_frame_budget(lite ? LITE_BUDK : FULL_BUDK);
    if (getenv("JD_HUD")) hud_on = 1;   /* the simulator has no three-finger tap */
#endif

    int dw = W, dh = H;
    SDL_GetRendererOutputSize(ren, &dw, &dh);      /* real pixels, HiDPI aware */
    fb_pick(dw, dh, &fb_w, &fb_h);
    framebuffer = (uint32_t *)calloc((size_t)fb_w * fb_h, 4);
    SDL_Texture *tex = SDL_CreateTexture(
        ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, fb_w, fb_h);

    /* JellyDazzleAudio: listen to whatever the Mac can hear.  If there is
     * no input device the engine simply runs on its own clocks. */
#if TARGET_OS_IPHONE
    /* iOS: draw first, ask for the mic without blocking, open it once granted
     * (see ios_host.m). JD_AUDIO_SRC=off skips the ask entirely. */
    int audio_up = 0;
    { const char *e = getenv("JD_AUDIO_SRC"); if (e && !SDL_strcasecmp(e, "off")) audio_up = 1; else jd_mic_request(); }
#else
    jd_audio_init();
#endif

    int running = 1, paused = 0;
    unsigned tick = 0;
    /* random launch seed: every run starts somewhere new in the wheel */
    srand((unsigned)time(NULL) ^ (unsigned)(getpid() * 2654435761u));
    int frame = rand() & 0x3FFFFF;
    /* JD_SEED=<n> replays a launch: everything downstream — run seed, bags,
     * palette epochs, the opening — derives from this number (3.4) */
    { const char *e = getenv("JD_SEED"); if (e && *e) frame = atoi(e) & 0x3FFFFF; }
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
#if !TARGET_OS_IPHONE
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                running = 0;
            /* 3.5.5 button bar: movement brightens it, a left click presses */
            if (e.type == SDL_MOUSEMOTION) ui_touch = SDL_GetTicks();
            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                int ww = 1, wh = 1; SDL_GetWindowSize(win, &ww, &wh);
                int b = bar_hit(e.button.x * (double)fb_w / ww, e.button.y * (double)fb_h / wh);
                if (b >= 0) bar_act(b, &running);
            }
            /* F (or cmd-F) toggles full screen */
            /* Bare F only. SDL's own View menu binds Ctrl-Cmd-F to fullscreen, and
             * AppKit fires that menu item AND still delivers the key here — so
             * without this guard Ctrl-Cmd-F toggles twice and does nothing. */
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_f
                && !(e.key.keysym.mod & (KMOD_GUI | KMOD_CTRL)))
                SDL_SetWindowFullscreen(win,
                    (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP)
                    ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
#else
            /* iOS kills apps that draw in the background; stop drawing until we
             * are back. The mic is suspended by the system with the app. */
            if (e.type == SDL_APP_WILLENTERBACKGROUND) paused = 1;
            if (e.type == SDL_APP_DIDENTERFOREGROUND)  paused = 0;
            touch_event(&e);
#endif
            /* resized, rotated or went full screen: re-render at the new real size */
            if (e.type == SDL_WINDOWEVENT &&
                (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                 e.window.event == SDL_WINDOWEVENT_RESIZED))
                fb_refit(ren, &tex);
        }
        if (paused) { SDL_Delay(100); continue; }

#if TARGET_OS_IPHONE
        touch_poll();
        /* Opened on a helper thread: AudioQueueStart can stall inside Apple's audio
         * server (it does in the simulator, indefinitely). On the main thread that
         * froze the picture; here the worst case is a show without a beat. */
        if (!audio_up && jd_mic_state() == 1) {
            SDL_Thread *t = SDL_CreateThread(audio_open_thread, "jd-mic", NULL);
            if (t) SDL_DetachThread(t);
            audio_up = 1;
        }
        /* Plugged in or not, Low Power Mode, heat: re-read every ~2 s. The switch is
         * live — resolution swaps between frames, the layer cap only gates NEW layers,
         * so nothing on screen is cut off mid-show. */
        if (res_req) {
            res_req = 0;
            if (lite) { lite_i = (lite_i + 1) % 3; lite_pix = LITE_SET[lite_i]; pix_cap = lite_pix; }
            else      { full_i = (full_i + 1) % 3; full_pix = FULL_SET[full_i]; pix_cap = full_pix; }
            fb_refit(ren, &tex);
        }
        if ((tick % 120) == 0) {
            int want = jd_power_lite();
            if (want != lite) {
                lite = want;
                pix_cap = lite ? lite_pix : full_pix;
                jd_set_slot_cap(lite ? LITE_SLOTS : FULL_SLOTS);
                jd_set_frame_budget(lite ? LITE_BUDK : FULL_BUDK);
                fb_refit(ren, &tex);
            }
        }
#endif
        tick++;

        jd_audio_tick();
        /* Lite skips the draw on odd refreshes (30 fps) and advances time by two
         * frames when it does draw, so the tempo is the same as Full. The old
         * picture is presented again on the skipped refresh — a GPU copy, nearly free. */
        if (!lite || (tick & 1) == 0) {
            jd_frame(framebuffer, fb_w, fb_h, frame);  /* <-- your assembly */
            frame += lite ? 2 : 1;
#if TARGET_OS_IPHONE
            /* drawn-frames per second, re-measured each second */
            { static Uint32 t0 = 0; static int n = 0; static double fps = 0; Uint32 t = SDL_GetTicks();
              n++; if (!t0) t0 = t; if (t - t0 >= 1000) { fps = n * 1000.0 / (t - t0); n = 0; t0 = t; }
              if (hud_on && !jd_about_is_on()) hud_draw(lite, fps);   /* the about card wins the screen */
              /* JD_PERFLOG: the HUD's numbers once a second on stdout, for A/B runs */
              static Uint32 tl = 0; if (getenv("JD_PERFLOG") && t - tl >= 1000) { tl = t;
                double ms = 0, bk = 1; int hot = 0, live = 0, cap = 0; jd_perf_get(&ms, &hot, &live, &cap, &bk);
                printf("PERF fps=%.0f ms=%.1f hot=%d live=%d cap=%d px=%dx%d\n", fps, ms, hot, live, cap, fb_w, fb_h); fflush(stdout); } }
#endif
                bar_draw();
            SDL_UpdateTexture(tex, NULL, framebuffer, fb_w * sizeof(uint32_t));
        }
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    jd_audio_close();
    SDL_Quit();
    return 0;
}
