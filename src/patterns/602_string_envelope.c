/* 602 String Envelope — THE DAZZLE METHOD.
 *
 * Everything else in this library repaints every pixel, every frame. This one
 * does not. It draws a composition ONCE into a plane of palette INDICES, and
 * then animates for its whole turn by moving the palette underneath it. The
 * geometry never moves. Only the colour travels.
 *
 * That is how DAZZLE.EXE worked, and why it looked so rich on an 8088: a pixel
 * in VGA mode 13h is an index into a 256-entry hardware palette, so rewriting
 * the palette recoloured the entire screen in microseconds without touching
 * video memory. Video of the original shows eight of twelve sampled frames
 * holding identical geometry with only the colour sweeping through it.
 *
 * The content is the original's workhorse primitive: LINE-SWEEP STRING ART.
 * Two endpoints each walk their own path, and straight lines are drawn between
 * them. Every line is straight; the ENVELOPE they collectively trace is a
 * curve — hyperbolas, bowties, hourglasses, with bright caustic edges where
 * the lines bunch. The palette index steps per line, so a swept family becomes
 * a travelling rainbow band rather than a flat colour.
 *
 * Cost: the draw happens once per turn. Every frame after that is one table
 * lookup per pixel, no maths, no branches — cheaper than anything else here.
 * And it cannot strobe: with the geometry fixed, the frame-to-frame delta is
 * bounded by how far the palette moved.
 *
 * Repaint-safe: regenerates whenever the seed or the framebuffer size changes.
 */
#include "../engine/jellydazzle.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define IDX_BG   0            /* index 0 is transparent (black) for blending */
#define NFAM     7            /* line families per composition              */

static uint8_t *pl602;                    /* the index plane */
static int      pw602, ph602;
static uint32_t seed602 = 0xFFFFFFFFu;

static uint32_t r602(uint32_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
    return *s;
}
static float f602(uint32_t *s) { return (float)(r602(s) >> 8) * (1.0f / 16777216.0f); }

/* one straight line into the index plane, drawn 4-way mirrored about the
 * centre so the whole thing reads as a kaleidoscope rather than scribble */
static void line602(float x0, float y0, float x1, float y1, uint8_t v, int mir)
{
    float dx = x1 - x0, dy = y1 - y0;
    float n = fabsf(dx) > fabsf(dy) ? fabsf(dx) : fabsf(dy);
    if (n < 1.0f) n = 1.0f;
    if (n > 4096.0f) n = 4096.0f;
    float sx = dx / n, sy = dy / n;
    float cx = (float)pw602 * 0.5f, cy = (float)ph602 * 0.5f;
    for (int i = 0; i <= (int)n; i++) {
        float px = x0 + sx * (float)i, py = y0 + sy * (float)i;
        for (int m = 0; m < 4; m++) {
            if (m && !(mir & m)) continue;
            float qx = (m & 1) ? cx * 2.0f - px : px;
            float qy = (m & 2) ? cy * 2.0f - py : py;
            int ix = (int)qx, iy = (int)qy;
            if (ix < 0 || iy < 0 || ix >= pw602 || iy >= ph602) continue;
            uint8_t *p = &pl602[(size_t)iy * pw602 + ix];
            if (*p < v) *p = v;          /* brighter index wins, like MAX blend */
        }
    }
}

/* a family of lines whose two endpoints each travel their own segment: the
 * lines are straight, the envelope is not */
static void sweep602(uint32_t *s)
{
    float w = (float)pw602, h = (float)ph602;
    /* each endpoint walks either a straight SEGMENT or an ARC — the arc is
     * what turns a bowtie into a rosette, and the original used both */
    int   ka = f602(s) < 0.45f, kb = f602(s) < 0.45f;   /* 1 = arc */
    float ax0 = f602(s) * w, ay0 = f602(s) * h;
    float ax1 = f602(s) * w, ay1 = f602(s) * h;
    float bx0 = f602(s) * w, by0 = f602(s) * h;
    float bx1 = f602(s) * w, by1 = f602(s) * h;
    float acx = w * (0.25f + 0.5f * f602(s)), acy = h * (0.25f + 0.5f * f602(s));
    float ar  = (w < h ? w : h) * (0.12f + 0.36f * f602(s));
    float aa0 = f602(s) * 6.2832f, aa1 = aa0 + (1.0f + 4.0f * f602(s));
    float bcx = w * (0.25f + 0.5f * f602(s)), bcy = h * (0.25f + 0.5f * f602(s));
    float br  = (w < h ? w : h) * (0.12f + 0.36f * f602(s));
    float ba0 = f602(s) * 6.2832f, ba1 = ba0 + (1.0f + 4.0f * f602(s));
    int   n    = 26 + (int)(f602(s) * 120.0f);      /* lines in the family */
    int   base = 24 + (int)(f602(s) * 180.0f);      /* where the ramp starts */
    int   step = 1  + (int)(f602(s) * 16.0f);       /* index step PER LINE  */
    int   mir  = 1 + (int)(f602(s) * 3.0f);         /* 1 = x, 2 = y, 3 = both */
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n;
        float px, py, qx, qy;
        if (ka) { float a = aa0 + (aa1 - aa0) * t;
                  px = acx + cosf(a) * ar; py = acy + sinf(a) * ar; }
        else    { px = ax0 + (ax1 - ax0) * t; py = ay0 + (ay1 - ay0) * t; }
        if (kb) { float a = ba0 + (ba1 - ba0) * t;
                  qx = bcx + cosf(a) * br; qy = bcy + sinf(a) * br; }
        else    { qx = bx0 + (bx1 - bx0) * t; qy = by0 + (by1 - by0) * t; }
        int v = base + i * step;
        v = 1 + (v & 254);                          /* wrap, never index 0 */
        line602(px, py, qx, qy, (uint8_t)v, mir);
    }
}

static void build602(int w, int h, uint32_t seed)
{
    size_t need = (size_t)w * h;
    if (pw602 != w || ph602 != h) {
        free(pl602); pl602 = (uint8_t *)malloc(need);
        pw602 = w; ph602 = h;
    }
    if (!pl602) return;
    memset(pl602, IDX_BG, need);
    uint32_t s = seed ? seed : 0x9E3779B9u;
    int fam = 4 + (int)(f602(&s) * (NFAM - 3));
    for (int i = 0; i < fam; i++) sweep602(&s);
}

void pattern_602(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (seed != seed602 || pw602 != w || ph602 != h) {
        build602(w, h, seed);
        seed602 = seed;
    }
    if (!pl602) return;
    /* the ONLY per-frame work: walk the palette under a fixed image.
     * ~34 s per full turn of the ramp — slow enough to read as a sweep. */
    uint32_t rot = (uint32_t)frame * 15u;
    uint32_t lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = i == IDX_BG ? 0xFF000000u
                             : pal[((uint32_t)i * 128u + rot) & JD_PAL_MASK];
    const uint8_t *src = pl602;
    size_t n = (size_t)w * h;
    for (size_t i = 0; i < n; i++) fb[i] = lut[src[i]];
}
