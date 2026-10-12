/* 099 Racetrack Drums — port of lab/patterns/099_racetrack_drums/proto.py
 * Full-screen concentric stadium/racetrack rings in thick multicolour bands
 * marching outward while their hues roll; inside the innermost oval two striped
 * drums roll their traffic-light ramps vertically and hot pink blobs ride the
 * perimeter. The stadium distance map is static; only band phase moves. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P99_LW 640
#define P99_LH 480
#define P99_N  (P99_LW * P99_LH)
#define P99_PI 3.14159265f

static uint32_t p99_low[P99_N];
static uint16_t p99_dist[P99_N];        /* stadium distance, 1/8 px units */
static float    p99_sin[2048];
static int      p99_ready;
/* 3.6.3 (J: "I see this shape over and over… no new colors"). It ignored its
 * seed, so every appearance was the same track, and its colours were a fixed
 * rainbow table only lightly tinted by the palette, so C barely touched it.
 * Now each appearance gets its own track (straight half-length, band width,
 * march direction) and every colour comes from the live palette. */
static uint32_t p99_seed = 0xFFFFFFFFu;
static float    p99_S = 64.0f;          /* straight half-length (0 = round) */
static float    p99_bw = 11.0f;         /* band width                       */
static float    p99_dir = 1.0f;         /* march outward (+1) or inward (-1) */
static uint32_t p99_base;               /* where on the ramp this run starts */

static const float p99_blob[8][2] = {
    { 110.0f, 62.0f }, { -110.0f, 62.0f }, { 110.0f, -62.0f }, { -110.0f, -62.0f },
    { 0.0f, 96.0f }, { 0.0f, -96.0f }, { 140.0f, 0.0f }, { -140.0f, 0.0f },
};

static uint32_t p99_h(uint32_t x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }

/* Drums and blobs are accents: a dark stretch of the ramp would draw them
 * nearly black (seen in testing). Keep the hue, lift the brightest channel
 * to at least 190. */
static uint32_t p99_lift(uint32_t u)
{
    uint32_t r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
    uint32_t mx = r > g ? r : g; if (b > mx) mx = b;
    if (mx >= 190) return u & 0xFFFFFFu;
    if (mx < 8) return 0xC0C0C0u;
    r = r * 190 / mx; g = g * 190 / mx; b = b * 190 / mx;
    return (r << 16) | (g << 8) | b;
}

static void p99_init(void)
{
    int i, x, y;
    for (i = 0; i < 2048; i++)
        p99_sin[i] = sinf((float)i * (2.0f * P99_PI / 2048.0f));
    for (y = 0; y < P99_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P99_LH);
        float dy = ly - 120.0f;
        for (x = 0; x < P99_LW; x++) {
            float lx = ((float)x + 0.5f) * (320.0f / (float)P99_LW);
            float dx = lx - 160.0f, ax = dx < 0.0f ? -dx : dx;
            float dxs = ax - p99_S; if (dxs < 0.0f) dxs = 0.0f;
            p99_dist[y * P99_LW + x] = (uint16_t)(sqrtf(dxs * dxs + dy * dy) * 8.0f);
        }
    }
    p99_ready = 1;
}

static inline float p99_lsin(float a)
{
    return p99_sin[((int)(a * 325.9493f + 2048.5f)) & 2047];
}

static void p99_tint(const uint32_t *pal, float *tn)
{
    float s[3] = { 0.0f, 0.0f, 0.0f }, mx;
    int i;
    for (i = 0; i < 64; i++) {
        uint32_t u = pal[(i * 512) & JD_PAL_MASK];
        s[0] += (float)((u >> 16) & 255);
        s[1] += (float)((u >> 8) & 255);
        s[2] += (float)(u & 255);
    }
    mx = s[0] > s[1] ? s[0] : s[1]; if (s[2] > mx) mx = s[2];
    if (mx < 1.0f) mx = 1.0f;
    for (i = 0; i < 3; i++) tn[i] = 0.85f + 0.15f * (s[i] / mx);
}

static uint32_t p99_pack(float r, float g, float b, const float *tn)
{
    int ri = (int)(r * tn[0]), gi = (int)(g * tn[1]), bi = (int)(b * tn[2]);
    if (ri > 255) ri = 255; if (gi > 255) gi = 255; if (bi > 255) bi = 255;
    if (ri < 0) ri = 0; if (gi < 0) gi = 0; if (bi < 0) bi = 0;
    return ((uint32_t)ri << 16) | ((uint32_t)gi << 8) | (uint32_t)bi;
}

static void p99_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P99_LW << 16) / w);
    int fx0 = (int)(((long)P99_LW << 15) / w) - (1 << 15);
    int maxx = (P99_LW - 1) << 16, maxy = (P99_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P99_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P99_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p99_low + (long)y0 * P99_LW;
        r1 = p99_low + (long)y1 * P99_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P99_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((cx >> 8) & 255), sx = 256u - wx, sy2 = 256u - (unsigned)wy;
            uint32_t a = r0[x0], b = r0[x1], c = r1[x0], d = r1[x1];
            uint32_t trb = (((a & 0xFF00FFu) * sx + (b & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t tg  = (((a & 0x00FF00u) * sx + (b & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t brb = (((c & 0xFF00FFu) * sx + (d & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t bg  = (((c & 0x00FF00u) * sx + (d & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t orb = ((trb * sy2 + brb * (unsigned)wy) >> 8) & 0xFF00FFu;
            uint32_t og  = ((tg  * sy2 + bg  * (unsigned)wy) >> 8) & 0x00FF00u;
            dst[x] = 0xFF000000u | orb | og;
            fx += stepx;
        }
    }
}

/* linear blend of two packed ARGB colours, f in 0..1 (F-099 edge AA) */
static inline uint32_t p99_mix(uint32_t a, uint32_t b, float f)
{
    int k = (int)(f * 256.0f);
    if (k < 0) k = 0; if (k > 256) k = 256;
    {
        uint32_t r = (((a >> 16) & 255u) * (uint32_t)(256 - k) + ((b >> 16) & 255u) * (uint32_t)k) >> 8;
        uint32_t g = (((a >>  8) & 255u) * (uint32_t)(256 - k) + ((b >>  8) & 255u) * (uint32_t)k) >> 8;
        uint32_t bl = ((a & 255u) * (uint32_t)(256 - k) + (b & 255u) * (uint32_t)k) >> 8;
        return 0xFF000000u | (r << 16) | (g << 8) | bl;
    }
}

void pattern_099(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float march, droll, tn[3], hroll;
    uint32_t bcol[8], scol[8], court, dedge;
    int i, x, y;
    (void)sl;

    if (seed != p99_seed || !p99_ready) {
        uint32_t a = p99_h(seed ^ 0x9917u), b = p99_h(a), c = p99_h(b);
        p99_S    = 24.0f + (float)(a % 1000u) * 0.080f;          /* 24..104: near-round to long */
        p99_bw   = 8.0f + (float)(b % 1000u) * 0.006f;           /* 8..14                       */
        p99_dir  = (c & 1u) ? 1.0f : -1.0f;
        p99_base = p99_h(c) & JD_PAL_MASK;
        p99_seed = seed;
        p99_init();                                             /* distance map for this track */
    }
    p99_tint(pal, tn);
    {   /* march = band phase; one full cycle is 8 bands of this run's width */
        float cyc = 8.0f * p99_bw;
        march = p99_dir * t * 0.18f; march -= cyc * floorf(march / cyc);
    }
    droll = t * 0.55f; droll -= 48.0f * floorf(droll / 48.0f);
    /* The hue roll advances one band every 50 frames.  It used to be taken
     * as an integer added to the band INDEX, which re-coloured all eight
     * bands at once — a whole-screen colour cut every 50 frames (measured
     * frame-delta ~80, far past the motion budget).
     *
     * Fold it into the band COORDINATE instead.  Each pixel now crosses
     * into the next band when its own v+hroll passes an integer, so the
     * recolour arrives as a boundary sweeping across the rings rather than
     * a global snap, and every band always shows a pure palette colour.
     * (Cross-fading the eight band colours pairwise was tried first and
     * rejected: lerping between opposite hues in RGB passes through grey,
     * so the whole image throbbed in and out of saturation.)
     *
     * Wrapped to the 8-band cycle so it stays exact over long runs. */
    hroll = t * 0.02f;
    hroll -= 8.0f * floorf(hroll * 0.125f);

    /* 8 band colours sampled evenly round the live ramp from this run's
     * start: C (a new scheme) repaints every band, and no two appearances
     * share a colour layout. The dark separator is the same hue at 35%. */
    for (i = 0; i < 8; i++) {
        uint32_t u = pal[(p99_base + (uint32_t)i * 4096u) & JD_PAL_MASK];
        bcol[i] = u & 0xFFFFFFu;
        scol[i] = p99_pack((float)((u >> 16) & 255) * 0.35f, (float)((u >> 8) & 255) * 0.35f,
                           (float)(u & 255) * 0.35f, tn);
    }
    {   uint32_t u = pal[(p99_base + 2048u) & JD_PAL_MASK];      /* court: the ramp, very dark */
        court = p99_pack((float)((u >> 16) & 255) * 0.16f, (float)((u >> 8) & 255) * 0.16f,
                         (float)(u & 255) * 0.16f, tn);
        dedge = p99_pack((float)((u >> 16) & 255) * 0.08f, (float)((u >> 8) & 255) * 0.08f,
                         (float)(u & 255) * 0.08f, tn); }

    for (y = 0; y < P99_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P99_LH);
        float dy = ly - 120.0f, ady = dy < 0.0f ? -dy : dy;
        const uint16_t *dr = p99_dist + (long)y * P99_LW;
        uint32_t *out = p99_low + (long)y * P99_LW;
        for (x = 0; x < P99_LW; x++) {
            float d = (float)dr[x] * 0.125f;
            float v = (d - march + 32.0f * p99_bw) * (1.0f / p99_bw) + hroll;
            int bi = (int)v;
            float fr = v - (float)bi;
            /* TEMPORAL REVIEW 2.4.0 (docs/review/04_pattern_temporal.md,
             * F-099): the band edges were hard cuts sampled on the distance
             * map's 0.125-px rings, so as the bands march (0.0164 v-units
             * per frame) whole iso-distance rings flipped colour together —
             * an irregular delta 0.2/0.8/2.3 shimmer.  A one-ring-wide
             * linear blend at each band edge lets every ring glide through
             * the boundary instead of snapping. */
            {
                const float aaw = 0.18f / p99_bw;   /* one frame of march, in v */
                uint32_t c;
                if (fr < aaw)
                    c = p99_mix(bcol[(bi + 7) & 7], scol[bi & 7], fr * (1.0f / aaw));
                else if (fr < 0.163636f)
                    c = scol[bi & 7];
                else if (fr < 0.163636f + aaw)
                    c = p99_mix(scol[bi & 7], bcol[bi & 7],
                                (fr - 0.163636f) * (1.0f / aaw));
                else
                    c = bcol[bi & 7];
                out[x] = c;
            }
            if (d < 34.0f) out[x] = court;
        }
        /* two striped drums inside the inner oval */
        if (ady < 26.0f) {
            int di = (int)floorf((ly - droll) * (1.0f / 6.0f));
            uint32_t dc;
            int k;
            di = ((di % 8) + 8) % 8;
            {   /* drum stripes walk a short stretch of the ramp, out and back */
                static const uint32_t step[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };
                uint32_t u = pal[(p99_base + 16384u + step[di] * 1400u) & JD_PAL_MASK];
                dc = p99_lift(u);
            }
            if (p99_S < 30.0f) goto no_drums;      /* near-round track: no room inside */
            for (k = 0; k < 2; k++) {
                float ks = p99_S / 64.0f;
                float x0 = (k ? 8.0f : -56.0f) * ks, x1 = (k ? 56.0f : -8.0f) * ks;
                int xa = (int)((160.0f + x0) * 2.0f), xb = (int)((160.0f + x1) * 2.0f);
                if (xa < 0) xa = 0; if (xb > P99_LW) xb = P99_LW;
                for (x = xa; x < xb; x++) {
                    float lx = ((float)x + 0.5f) * 0.5f, dx = lx - 160.0f;
                    float e0 = dx - x0, e1 = dx - x1;
                    if (e0 < 0.0f) e0 = -e0; if (e1 < 0.0f) e1 = -e1;
                    out[x] = (e0 < 1.5f || e1 < 1.5f) ? dedge : dc;
                }
            }
            no_drums: ;
        }
        /* hot pink perimeter blobs */
        for (i = 0; i < 8; i++) {
            float by = p99_blob[i][1], bx = p99_blob[i][0] * ((p99_S + 76.0f) / 140.0f);
            float ey = dy - by, q;
            uint32_t bc;
            int xa, xb;
            if (ey * ey >= 55.0f) continue;
            q = sqrtf(90.0f * (1.0f - ey * ey / 55.0f));
            {
                float pl = 0.7f + 0.3f * p99_lsin(t * 0.02f - 2.0f * P99_PI
                           * floorf(t * 0.02f / (2.0f * P99_PI)) + bx * 0.05f + by * 0.03f);
                uint32_t u = p99_lift(pal[(p99_base + 18432u) & JD_PAL_MASK]);
                bc = p99_pack((float)((u >> 16) & 255) * pl, (float)((u >> 8) & 255) * pl,
                              (float)(u & 255) * pl, tn);
            }
            xa = (int)((160.0f + bx - q) * 2.0f); xb = (int)((160.0f + bx + q) * 2.0f);
            if (xa < 0) xa = 0; if (xb > P99_LW) xb = P99_LW;
            for (x = xa; x < xb; x++) out[x] = bc;
        }
    }
    p99_blit(fb, w, h);
}
