/* _xfade.h — continuous morph for progressively rendered fractals (3.6.1).
 *
 * J, 2026-10-12: the fractals "flash the changes instead of letting the flow
 * evolve freely". Measured cause: they render a band of rows per frame.
 *  - gk-canvas fractals (629-633, 638) DECAY the whole canvas every frame
 *    and redraw one band, so the band just drawn is ~2x brighter than the
 *    band about to be redrawn: a bright bar rolls down the screen 6-12 times
 *    a second.
 *  - index-plane fractals (605, 607) overwrite rows in place with the NEXT
 *    parameter set, so a seam between old and new wipes down every sweep.
 *
 * The fix is the same idea for both: never show a half-built image. Each
 * sweep builds a complete picture off-screen; the screen shows the last TWO
 * complete pictures crossfaded LINEARLY over exactly one sweep, so the blend
 * reaches the newest just as the next one lands. Linear, not eased: an eased
 * blend per segment would pulse the speed of the morph every sweep.
 *
 * The picture runs one sweep (~0.1-1.5 s) behind the maths. Nobody can see
 * that; everybody could see the bands. */
#ifndef JD_XFADE_H
#define JD_XFADE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---- index planes (605, 607) ------------------------------------------ */
typedef struct {
    uint8_t *a, *b;          /* a = previous complete plane, b = newest     */
    size_t   n;
    int      have;           /* complete planes held: 0, 1 or 2             */
    int      age, len;       /* frames since b landed, frames per sweep     */
} jx_xf;

static void jx_reset(jx_xf *x) { x->have = 0; x->age = 0; }

/* A sweep finished: `plane` is a complete image. */
static void jx_commit(jx_xf *x, const uint8_t *plane, size_t n, int len)
{
    if (x->n != n || !x->a || !x->b) {
        free(x->a); free(x->b);
        x->a = (uint8_t *)malloc(n); x->b = (uint8_t *)malloc(n);
        x->n = (x->a && x->b) ? n : 0;
        x->have = 0;
        if (!x->n) return;
    }
    uint8_t *t = x->a; x->a = x->b; x->b = t;
    memcpy(x->b, plane, n);
    if (x->have < 2) x->have++;
    x->age = 0; x->len = len > 1 ? len : 1;
}

/* Colour the screen. Before the first sweep completes there is nothing
 * finished to show, so the live plane is shown as it builds (the layer is
 * fading in from zero then, which hides the wipe). */
static void jx_present(jx_xf *x, const uint8_t *live, uint32_t *fb, size_t n,
                       const uint32_t *lut)
{
    if (x->have == 0 || x->n != n) { for (size_t i = 0; i < n; i++) fb[i] = lut[live[i]]; return; }
    if (x->have == 1)               { for (size_t i = 0; i < n; i++) fb[i] = lut[x->b[i]]; return; }
    uint32_t f = (uint32_t)(x->age * 256 / x->len);
    if (f > 256) f = 256;
    if (x->age < x->len) x->age++;
    uint32_t g = 256 - f;
    const uint8_t *A = x->a, *B = x->b;
    for (size_t i = 0; i < n; i++) {
        uint32_t ca = lut[A[i]], cb = lut[B[i]];
        if (ca == cb) { fb[i] = ca; continue; }
        uint32_t rb = (((ca & 0xFF00FFu) * g + (cb & 0xFF00FFu) * f) >> 8) & 0xFF00FFu;
        uint32_t gg = (((ca & 0x00FF00u) * g + (cb & 0x00FF00u) * f) >> 8) & 0x00FF00u;
        fb[i] = 0xFF000000u | rb | gg;
    }
}

/* ---- gk float canvases (629-633, 638) -------------------------------- */
#ifdef GK_TAU
typedef struct {
    float *a, *b;
    int    n;                /* floats per canvas (cw*ch*3)                 */
    int    have, age, len;
} gk_xf;

static void gk_xf_reset(gk_xf *x) { x->have = 0; x->age = 0; }

static void gk_xf_commit(gk_xf *x, const gk *g, int len)
{
    int n = g->cw * g->ch * 3;
    if (x->n != n || !x->a || !x->b) {
        free(x->a); free(x->b);
        x->a = (float *)malloc(sizeof(float) * (size_t)n);
        x->b = (float *)malloc(sizeof(float) * (size_t)n);
        x->n = (x->a && x->b) ? n : 0;
        x->have = 0;
        if (!x->n) return;
    }
    float *t = x->a; x->a = x->b; x->b = t;
    memcpy(x->b, g->acc, sizeof(float) * (size_t)n);
    if (x->have < 2) x->have++;
    x->age = 0; x->len = len > 1 ? len : 1;
}

/* Tone-map the crossfade of the last two finished canvases (same curve as
 * gk_present), or the live canvas until the first sweep is done. */
static void gk_xf_present(gk_xf *x, gk *g, uint32_t *fb, int w, int h)
{
    int n = g->cw * g->ch * 3;
    if (x->have == 0 || x->n != n) { gk_present(g, fb, w, h); return; }
    float f = x->have == 1 ? 1.0f : (float)x->age / (float)x->len;
    if (f > 1.0f) f = 1.0f;
    if (x->have == 2 && x->age < x->len) x->age++;
    const float *A = x->a, *B = x->b;
    uint8_t *o = g->img;
    for (int i = 0; i < n; i++) {
        float v = A[i] + (B[i] - A[i]) * f;
        int t = (int)(v * 512.0f);
        o[i] = gk_ttab[t < 0 ? 0 : t > 4096 ? 4096 : t];
    }
    jd_up_blit(&g->up, fb, w, h, g->img, g->cw, g->ch);
}
#endif

#endif
