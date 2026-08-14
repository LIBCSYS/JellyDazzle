/* jd_up.h — shared bilinear upscale for the "render small, blit big" patterns.
 *
 * Roughly a third of the library builds an internal RGB canvas of a few
 * hundred pixels a side and bilinear-upscales it to the framebuffer.  The
 * copy-pasted loop that did this resampled every source row once per OUTPUT
 * row that read it — at 1280x960 over a 156-row canvas that is each source
 * row resampled six times over — and it computed the source column with a
 * 64-bit multiply and an integer DIVIDE per pixel.
 *
 * This does the identical arithmetic with the horizontal half hoisted:
 * the column geometry becomes a table built once per output width, and the
 * horizontally-resampled source rows are cached and stepped as y advances,
 * leaving a two-tap vertical lerp per pixel.
 *
 * BIT-IDENTICAL, not merely similar.  The expressions below are the originals
 * verbatim; only their home moved.  The intermediate `top`/`bot` provably fit
 * in a uint8: with d = s1 - s0 and fx in [0,255], s0 + ((d*fx)>>8) lies in
 * [0,254] for any s0,s1 in [0,255], so caching them as bytes loses nothing.
 *
 * Everything here is static — nothing but the pattern_NNN entry point ever
 * escapes a pattern translation unit.
 */
#ifndef JD_UP_H
#define JD_UP_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    int      w, sw, sh;      /* geometry the tables were built for        */
    int     *o0, *o1;        /* byte offsets of the two source columns    */
    uint8_t *fx;             /* horizontal fraction, 0..255               */
    uint8_t *row0, *row1;    /* horizontally-resampled cache, 3*w each    */
    int      c0, c1;         /* source rows held in row0/row1, -1 = none  */
} jd_up;

/* build (or rebuild) the column tables and row cache for this geometry */
static int jd_up_prep(jd_up *u, int w, int sw, int sh)
{
    if (u->w == w && u->sw == sw && u->sh == sh && u->o0) return 1;
    free(u->o0); free(u->o1); free(u->fx); free(u->row0); free(u->row1);
    u->o0 = u->o1 = NULL; u->fx = u->row0 = u->row1 = NULL;
    u->w = 0; u->sw = 0; u->sh = 0;
    if (w <= 0 || sw <= 1 || sh <= 0) return 0;
    u->o0   = (int *)malloc(sizeof(int) * (size_t)w);
    u->o1   = (int *)malloc(sizeof(int) * (size_t)w);
    u->fx   = (uint8_t *)malloc((size_t)w);
    u->row0 = (uint8_t *)malloc((size_t)w * 3);
    u->row1 = (uint8_t *)malloc((size_t)w * 3);
    if (!u->o0 || !u->o1 || !u->fx || !u->row0 || !u->row1) {
        free(u->o0); free(u->o1); free(u->fx); free(u->row0); free(u->row1);
        u->o0 = u->o1 = NULL; u->fx = u->row0 = u->row1 = NULL;
        return 0;
    }
    for (int x = 0; x < w; x++) {
        int sx = (int)(((long long)x * (sw - 1) << 8) / (w > 1 ? w - 1 : 1));
        int x0 = sx >> 8;
        u->fx[x] = (uint8_t)(sx & 255);
        u->o0[x] = x0 * 3;
        u->o1[x] = (x0 + 1 < sw ? x0 + 1 : sw - 1) * 3;
    }
    u->w = w; u->sw = sw; u->sh = sh; u->c0 = u->c1 = -1;
    return 1;
}

/* horizontally resample source row `sy` into `dst` (3*w bytes) */
static void jd_up_hrow(const jd_up *u, const uint8_t *img, int sy, uint8_t *dst)
{
    const uint8_t *r = img + (size_t)sy * (size_t)u->sw * 3;
    const int *o0 = u->o0, *o1 = u->o1;
    const uint8_t *fxv = u->fx;
    int w = u->w;
    for (int x = 0; x < w; x++) {
        int a = o0[x], b = o1[x], f = fxv[x];
        dst[x*3+0] = (uint8_t)(r[a+0] + (((r[b+0] - r[a+0]) * f) >> 8));
        dst[x*3+1] = (uint8_t)(r[a+1] + (((r[b+1] - r[a+1]) * f) >> 8));
        dst[x*3+2] = (uint8_t)(r[a+2] + (((r[b+2] - r[a+2]) * f) >> 8));
    }
}

/* the slow path this replaces, kept verbatim as the allocation-failure
 * fallback so a failed malloc costs speed and never correctness */
static void jd_up_blit_slow(uint32_t *fb, int w, int h,
                            const uint8_t *img, int sw, int sh)
{
    for (int y = 0; y < h; y++) {
        int sy = (int)(((long long)y * (sh - 1) << 8) / (h > 1 ? h - 1 : 1));
        int y0 = sy >> 8, fy = sy & 255;
        int y1 = y0 + 1 < sh ? y0 + 1 : sh - 1;
        const uint8_t *r0 = img + (size_t)y0 * (size_t)sw * 3;
        const uint8_t *r1 = img + (size_t)y1 * (size_t)sw * 3;
        uint32_t *dst = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) {
            int sx = (int)(((long long)x * (sw - 1) << 8) / (w > 1 ? w - 1 : 1));
            int x0 = sx >> 8, fx = sx & 255;
            int x1 = x0 + 1 < sw ? x0 + 1 : sw - 1;
            int a = x0 * 3, b = x1 * 3, out[3];
            for (int c = 0; c < 3; c++) {
                int t = r0[a+c] + (((r0[b+c] - r0[a+c]) * fx) >> 8);
                int m = r1[a+c] + (((r1[b+c] - r1[a+c]) * fx) >> 8);
                out[c] = t + (((m - t) * fy) >> 8);
            }
            dst[x] = 0xFF000000u | ((uint32_t)out[0] << 16) |
                     ((uint32_t)out[1] << 8) | (uint32_t)out[2];
        }
    }
}

/* upscale a sw x sh RGB byte canvas into a w x h ARGB framebuffer */
static void jd_up_blit(jd_up *u, uint32_t *fb, int w, int h,
                       const uint8_t *img, int sw, int sh)
{
    if (!jd_up_prep(u, w, sw, sh)) { jd_up_blit_slow(fb, w, h, img, sw, sh); return; }
    /* the canvas is redrawn every frame, so the cache is per-call */
    u->c0 = u->c1 = -1;
    for (int y = 0; y < h; y++) {
        int sy = (int)(((long long)y * (sh - 1) << 8) / (h > 1 ? h - 1 : 1));
        int y0 = sy >> 8, fy = sy & 255;
        int y1 = y0 + 1 < sh ? y0 + 1 : sh - 1;
        if (y0 != u->c0 && y0 == u->c1) {       /* stepped on by one row */
            uint8_t *tmp = u->row0; u->row0 = u->row1; u->row1 = tmp;
            u->c0 = y0; u->c1 = -1;             /* row1 now holds a stale row */
        }
        if (y0 != u->c0) { jd_up_hrow(u, img, y0, u->row0); u->c0 = y0; }
        if (y1 != u->c1) {
            if (y1 == y0) { for (int k = 0; k < w * 3; k++) u->row1[k] = u->row0[k]; }
            else jd_up_hrow(u, img, y1, u->row1);
            u->c1 = y1;
        }
        const uint8_t *a = u->row0, *b = u->row1;
        uint32_t *dst = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) {
            int i = x * 3;
            int r = a[i+0] + (((b[i+0] - a[i+0]) * fy) >> 8);
            int g = a[i+1] + (((b[i+1] - a[i+1]) * fy) >> 8);
            int bl= a[i+2] + (((b[i+2] - a[i+2]) * fy) >> 8);
            dst[x] = 0xFF000000u | ((uint32_t)r << 16) |
                     ((uint32_t)g << 8) | (uint32_t)bl;
        }
    }
}

#endif /* JD_UP_H */
