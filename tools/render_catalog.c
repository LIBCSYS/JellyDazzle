/* anim2.c — render a pattern the way the ENGINE shows it, not the way a bare
 * harness does.
 *
 * The old renderer handed each pattern the whole 32768-entry ramp, so every
 * thumbnail came out as a full rainbow. The engine never does that: a layer
 * gets a WINDOW into the ramp (span ~4500..32768, typically 14000-20000) at
 * some offset, which is what makes a layer read as a coherent hue family
 * instead of a spectrum. Narrow windows are then reshaped — the value range
 * stretched and saturation lifted up to 1.5x — which is where the punch comes
 * from. That is why the app looked better than its own catalogue.
 *
 * This mirrors layer_pal_build() in src/engine/compositor.c.
 *
 *   anim2 <schemeA> <schemeB> <span> <off> <outdir>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "jellydazzle.h"
#define W 320
#define H 240
#define NCAP 40
#define WARM 820
#define PAL_N   32768
#define PAL_MASK 0x7FFF
#define PAL_RET 2048
void PATTERN(uint32_t*, int, int, int, int, uint32_t, const uint32_t*);
static uint32_t fb[W*H], pa[PAL_N], pb[PAL_N], blend[PAL_N], pal[PAL_N];

static void window_and_reshape(uint32_t span, uint32_t off)
{
    /* 1. the window — exactly the engine's cyclic build */
    if (span >= PAL_N) {
        for (uint32_t i = 0; i < PAL_N; i++)
            pal[i] = blend[(((i * span) >> 15) + off) & PAL_MASK];
    } else {
        const uint32_t body = PAL_N - PAL_RET;
        const uint32_t mul  = (span << 16) / body;
        const uint32_t top  = ((body - 1) * mul) >> 16;
        for (uint32_t i = 0; i < body; i++)
            pal[i] = blend[(((i * mul) >> 16) + off) & PAL_MASK];
        for (uint32_t j = 0; j < PAL_RET; j++)
            pal[body + j] = blend[(off + top - (top * (j + 1)) / PAL_RET) & PAL_MASK];
    }
    /* 2. the reshape — only bites on narrower windows, same as the engine */
    float k = 1.0f - (float)span / 24000.0f;
    if (k <= 0.02f) return;
    if (k > 0.85f) k = 0.85f;
    int lo = 255, hi = 0; long ssum = 0; int n = 0;
    for (int i = 0; i < PAL_N; i += 64) {
        uint32_t c = pal[i];
        int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
        int v = r > g ? (r > b ? r : b) : (g > b ? g : b);
        int m = r < g ? (r < b ? r : b) : (g < b ? g : b);
        if (v < lo) lo = v; if (v > hi) hi = v; ssum += v - m; n++;
    }
    if (hi - lo < 123) hi = lo + 123;
    float ms = (float)ssum / (n ? n : 1) / 255.0f;
    float gs = ms > 0.02f ? 0.38f / ms : 1.0f;
    if (gs < 1.0f) gs = 1.0f;
    if (gs > 1.5f) gs = 1.5f;
    gs = 1.0f + (gs - 1.0f) * k;
    uint32_t gq = (uint32_t)(gs * 256.0f);
    uint8_t lut[256];
    for (int v = 0; v < 256; v++) {
        float t = (float)(v - lo) / (float)(hi - lo);
        if (t < 0) t = 0; if (t > 1) t = 1;
        float o = (float)v + (6.0f + t * 245.0f - (float)v) * k;
        lut[v] = (uint8_t)(o < 0 ? 0 : o > 255 ? 255 : o);
    }
    static uint16_t recip[256];
    if (!recip[1]) for (int i = 1; i < 256; i++) recip[i] = (uint16_t)(65535 / i);
    for (int i = 0; i < PAL_N; i++) {
        uint32_t c = pal[i];
        uint32_t r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
        uint32_t v = r > g ? (r > b ? r : b) : (g > b ? g : b);
        uint32_t m = r < g ? (r < b ? r : b) : (g < b ? g : b);
        uint32_t sp = v - m, nv = lut[v];
        if (!sp) { pal[i] = 0xFF000000u | (nv << 16) | (nv << 8) | nv; continue; }
        uint32_t ns = (sp * gq) >> 8; if (ns > nv) ns = nv;
        uint32_t inv = ns * recip[sp], base = nv - ns;
        uint32_t nr = base + (((r - m) * inv) >> 16);
        uint32_t ng = base + (((g - m) * inv) >> 16);
        uint32_t nb = base + (((b - m) * inv) >> 16);
        pal[i] = 0xFF000000u | (nr > 255 ? 255u : nr) << 16
                             | (ng > 255 ? 255u : ng) << 8
                             | (nb > 255 ? 255u : nb);
    }
}

int main(int argc, char **argv)
{
    if (argc < 6) { fprintf(stderr, "anim2 A B span off outdir\n"); return 1; }
    int sa = atoi(argv[1]), sb = atoi(argv[2]);
    uint32_t span = (uint32_t)atoi(argv[3]), off = (uint32_t)atoi(argv[4]);
    const char *dir = argv[5];
    FILE *f = fopen("assets/palette.bin", "rb");
    if (!f) { fprintf(stderr, "no palette\n"); return 1; }
    fseek(f, (long)sa * PAL_N * 4, SEEK_SET); fread(pa, 4, PAL_N, f);
    fseek(f, (long)sb * PAL_N * 4, SEEK_SET); fread(pb, 4, PAL_N, f);
    fclose(f);
    uint32_t seed = 0xC0FFEE11u;
    for (int i = 0; i < PAL_N; i++) blend[i] = pa[i];
    window_and_reshape(span, off);
    for (int fr = 0; fr < WARM; fr++) PATTERN(fb, W, H, fr, fr & 2047, seed, pal);
    for (int c = 0; c < NCAP; c++) {
        int t = (c * 256) / NCAP;
        for (int i = 0; i < PAL_N; i++) {           /* crossfade the SOURCE */
            uint32_t x = pa[i], y = pb[i];
            uint32_t r = ((((x>>16)&255)*(256-t) + ((y>>16)&255)*t) >> 8);
            uint32_t g = ((((x>>8)&255)*(256-t) + ((y>>8)&255)*t) >> 8);
            uint32_t b = (((x&255)*(256-t) + (y&255)*t) >> 8);
            blend[i] = 0xFF000000u | (r<<16) | (g<<8) | b;
        }
        window_and_reshape(span, off + (uint32_t)c * 90);   /* and rotate it */
        int fr = WARM + c * 2;
        PATTERN(fb, W, H, fr, fr & 2047, seed, pal);
        char p[512]; snprintf(p, sizeof p, "%s/f%03d.ppm", dir, c);
        FILE *o = fopen(p, "wb"); fprintf(o, "P6\n%d %d\n255\n", W, H);
        for (int i = 0; i < W*H; i++) {
            uint32_t q = fb[i];
            fputc((q>>16)&255,o); fputc((q>>8)&255,o); fputc(q&255,o);
        }
        fclose(o);
    }
    return 0;
}
