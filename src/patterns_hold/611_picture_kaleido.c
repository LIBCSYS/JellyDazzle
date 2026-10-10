/* pattern_611 — PICTURE KALEIDO (3.4): your own picture, folded.
 *
 * The photo arrives from src/app/jd_image.m as two 512x512 planes — contrast-
 * stretched luminance and Sobel edges — never as colour.  Here it is read
 * through an N-way kaleidoscope fold (4/6/8/12 wedges, seeded), a slow turn
 * and a breathing zoom, and every output pixel is a PALETTE index: luma picks
 * the hue along the current scheme, edges push it along and brighten it.  So
 * the picture's shape is on screen wearing the engine's colours, re-tints with
 * the C key and the audio rotation, and belongs to the frame instead of
 * sitting on it.  With no picture loaded the mark (_emblem.h) stands in, so
 * the routine always has something to show the probe.
 *
 * Rendered on the 320x240 ground-kit canvas and upscaled like every other
 * background, so it costs what a soft kaleido costs. */
#include "_gk336.h"
#include "_emblem.h"

extern const uint8_t *jd_img_luma;
extern const uint8_t *jd_img_edge;
extern int jd_img_ready;
#define IMG_N 512

static inline float samp(const uint8_t *p, int n, float u, float v)
{
    /* bilinear, u/v in 0..1 */
    float fx = u * (float)(n - 1), fy = v * (float)(n - 1);
    int x0 = (int)fx, y0 = (int)fy;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x0 > n - 2) x0 = n - 2; if (y0 > n - 2) y0 = n - 2;
    float tx = fx - (float)x0, ty = fy - (float)y0;
    const uint8_t *r0 = p + y0 * n + x0, *r1 = r0 + n;
    float a = (float)r0[0] + ((float)r0[1] - (float)r0[0]) * tx;
    float b = (float)r1[0] + ((float)r1[1] - (float)r1[0]) * tx;
    return (a + (b - a) * ty) * (1.0f / 255.0f);
}

void pattern_611(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    const uint8_t *L = jd_img_ready ? jd_img_luma : jd_emblem;
    const uint8_t *E = jd_img_ready ? jd_img_edge : NULL;
    const int n = jd_img_ready ? IMG_N : JD_EMB_N;

    float t = (float)frame * 0.0011f;
    float hue0 = gk_sf(seed, 21) + t * 0.006f;
    static const int FOLDS[4] = { 4, 6, 8, 12 };
    int folds = FOLDS[(int)(gk_sf(seed, 22) * 3.999f)];
    float wseg = 6.2832f / (float)folds;
    float spin = t * (0.18f + 0.1f * gk_sf(seed, 23)) * (gk_sf(seed, 24) < 0.5f ? 1.0f : -1.0f);
    /* breathing zoom on the SOURCE window, so the fold keeps finding new edges */
    float zoom = 0.62f + 0.22f * gk_sin(t * 0.9f + gk_sf(seed, 25) * 6.28f);
    float ox = 0.5f + 0.18f * gk_sin(t * 0.37f), oy = 0.5f + 0.18f * gk_cos(t * 0.29f);
    float cx = GK_W * 0.5f, cy = GK_H * 0.5f;
    float rs = 1.0f / (float)(GK_H / 2);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy) * rs;          /* 0 at centre */
            float a = atan2f(dy, dx) + spin;
            float fa = fmodf(a + 20.0f * wseg, wseg);
            if (fa > wseg * 0.5f) fa = wseg - fa;             /* mirror wedge */
            float u = ox + r * gk_cos(fa) * zoom, v = oy + r * gk_sin(fa) * zoom;
            /* reflect at the picture's edges rather than clamp: no hard border */
            u = gk_absf(u); v = gk_absf(v);
            if (u > 1.0f) u = 2.0f - u; if (v > 1.0f) v = 2.0f - v;
            u = gk_clamp01(u); v = gk_clamp01(v);
            float lum = samp(L, n, u, v);
            float edg = E ? samp(E, n, u, v) : 0.0f;
            uint32_t c = gk_pal(pal, hue0 + lum * 0.42f + edg * 0.22f + r * 0.03f);
            float bright = 0.30f + 0.62f * powf(lum, 1.25f) + 0.30f * edg;
            if (bright > 1.0f) bright = 1.0f;
            gk_put(y * GK_W + x, gk_shade(c, bright));
        }
    }
    gk_blit(fb, w, h);
}
