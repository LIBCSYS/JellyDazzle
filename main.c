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
#include <time.h>
#include <unistd.h>

#ifndef JD_VERSION
#define JD_VERSION "dev"   /* set by the Makefile from ./VERSION */
#endif

#define W 1280
#define H 960

/* implemented in draw.s */
extern void jd_frame(uint32_t *fb, int width, int height, int frame);
extern int  jd_audio_init(void);
extern void jd_audio_tick(void);
extern void jd_audio_close(void);

static uint32_t framebuffer[W * H];   /* our A000:0000 */

int main(void)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *win = SDL_CreateWindow(
        "JellyDazzle v" JD_VERSION,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H, SDL_WINDOW_RESIZABLE);

    SDL_Renderer *ren = SDL_CreateRenderer(
        win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_Texture *tex = SDL_CreateTexture(
        ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, W, H);

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
        }

        jd_audio_tick();
        jd_frame(framebuffer, W, H, frame++);  /* <-- your assembly */

        SDL_UpdateTexture(tex, NULL, framebuffer, W * sizeof(uint32_t));
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
