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
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

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

/* implemented in draw.s */
extern void jd_frame(uint32_t *fb, int width, int height, int frame);
extern int  jd_audio_init(void);
extern void jd_audio_tick(void);
extern void jd_audio_close(void);

static uint32_t *framebuffer;         /* our A000:0000, sized to the window */
static int fb_w, fb_h;

/* choose the render size for a given drawable size */
static void fb_pick(int dw, int dh, int *rw, int *rh)
{
    if (dw < 64) dw = 64;
    if (dh < 64) dh = 64;
    double scale = 1.0;
    double area = (double)dw * dh;
    if (area > MAX_PIX) scale = sqrt(MAX_PIX / area);
    *rw = (int)(dw * scale) & ~1;
    *rh = (int)(dh * scale) & ~1;
}

int main(void)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *win = SDL_CreateWindow(
        "JellyDazzle v" JD_VERSION,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_Renderer *ren = SDL_CreateRenderer(
        win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    int dw = W, dh = H;
    SDL_GetRendererOutputSize(ren, &dw, &dh);      /* real pixels, HiDPI aware */
    fb_pick(dw, dh, &fb_w, &fb_h);
    framebuffer = (uint32_t *)calloc((size_t)fb_w * fb_h, 4);
    SDL_Texture *tex = SDL_CreateTexture(
        ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, fb_w, fb_h);

    /* JellyDazzleAudio: listen to whatever the Mac can hear.  If there is
     * no input device the engine simply runs on its own clocks. */
    jd_audio_init();

    int running = 1;
    /* random launch seed: every run starts somewhere new in the wheel */
    srand((unsigned)time(NULL) ^ (unsigned)(getpid() * 2654435761u));
    int frame = rand() & 0x3FFFFF;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT ||
                (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                running = 0;
            /* F (or cmd-F) toggles full screen */
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_f)
                SDL_SetWindowFullscreen(win,
                    (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP)
                    ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
            /* resized or went full screen: re-render at the new real size */
            if (e.type == SDL_WINDOWEVENT &&
                (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                 e.window.event == SDL_WINDOWEVENT_RESIZED)) {
                int ndw = fb_w, ndh = fb_h, nw, nh;
                SDL_GetRendererOutputSize(ren, &ndw, &ndh);
                fb_pick(ndw, ndh, &nw, &nh);
                if (nw != fb_w || nh != fb_h) {
                    uint32_t *nb = (uint32_t *)calloc((size_t)nw * nh, 4);
                    SDL_Texture *nt = SDL_CreateTexture(ren,
                        SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, nw, nh);
                    if (nb && nt) {
                        free(framebuffer); SDL_DestroyTexture(tex);
                        framebuffer = nb; tex = nt; fb_w = nw; fb_h = nh;
                    } else { free(nb); if (nt) SDL_DestroyTexture(nt); }
                }
            }
        }

        jd_audio_tick();
        jd_frame(framebuffer, fb_w, fb_h, frame++);  /* <-- your assembly */

        SDL_UpdateTexture(tex, NULL, framebuffer, fb_w * sizeof(uint32_t));
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
