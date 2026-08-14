/* bridge.c — the wheel dispatcher: 24 assembly modes + the lab patterns */
#include <stdint.h>
#include <stdlib.h>
#include "jellydazzle.h"

uint32_t g_mode = 0;                       /* read by draw.s mode select */
extern void draw_frame(uint32_t*, int, int, int);
extern const uint32_t jd_palette[];        /* 30*32768 ARGB, in draw.s */
extern const jd_pattern_fn jd_patterns[];  /* registry.c, indexed 0..N-1 */
extern const int jd_pattern_count;

static uint32_t mix32(uint32_t x) {
    x *= 0x9E3779B1u; x ^= x >> 16;
    x *= 0x85EBCA6Bu; x ^= x >> 13;
    return x;
}

static uint32_t blended[32768];

void jd_frame(uint32_t *fb, int w, int h, int frame) {
    static int override = -2;              /* JD_MODE: test any number */
    if (override == -2) {
        const char *ov = getenv("JD_MODE");
        override = ov ? atoi(ov) : -1;
    }
    int total = 24 + jd_pattern_count;
    uint32_t seg = (uint32_t)frame >> 11;
    int sl = frame & 2047;
    int m = (int)(mix32(seg) % (uint32_t)total);
    int pm = (int)(mix32(seg - 1) % (uint32_t)total);
    if (m == pm) m = (m + 7) % total;
    if (override >= 0) {
        /* JD_MODE=N: 1..jd_pattern_count = lab pattern N; 100+: asm mode N-100 */
        if (override >= 1 && override <= jd_pattern_count) m = 24 + override - 1;
        else m = override % 24;             /* JD_MODE=0 or 101.. -> asm modes */
    }
    if (m < 24) { g_mode = (uint32_t)m; draw_frame(fb, w, h, frame); return; }

    /* scheme-blended palette for lab patterns (own dice stream) */
    int A = (int)(mix32(seg * 2654435761u + 1) % 30);
    int B = (int)(mix32(seg * 2654435761u + 2) % 30);
    if (B == A) B = (B + 1) % 30;
    uint32_t t = (uint32_t)((sl >> 3) & 255), it = 256 - t;
    const uint32_t *pa = jd_palette + (size_t)A * 32768;
    const uint32_t *pb = jd_palette + (size_t)B * 32768;
    for (int i = 0; i < 32768; i++) {
        uint32_t ca = pa[i], cb = pb[i];
        uint32_t rb = (((ca & 0xFF00FFu) * it + (cb & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
        uint32_t g  = (((ca & 0x00FF00u) * it + (cb & 0x00FF00u) * t) >> 8) & 0x00FF00u;
        blended[i] = 0xFF000000u | rb | g;
    }
    jd_patterns[m - 24](fb, w, h, frame, sl, mix32(seg), blended);
}
