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

static const float p99_band[8][3] = {
    { 150.0f, 240.0f,  30.0f }, { 240.0f, 230.0f,  40.0f },
    {  30.0f,  90.0f, 220.0f }, { 200.0f,  40.0f,  60.0f },
    {  60.0f, 200.0f, 180.0f }, { 240.0f, 130.0f,  30.0f },
    {  40.0f,  40.0f, 160.0f }, { 120.0f, 230.0f,  90.0f },
};
static const float p99_drum[8][3] = {
    {  40.0f, 220.0f,  60.0f }, { 170.0f, 240.0f,  50.0f },
    { 250.0f, 210.0f,  40.0f }, { 250.0f, 120.0f,  40.0f },
    { 230.0f,  40.0f,  50.0f }, { 250.0f, 120.0f,  40.0f },
    { 250.0f, 210.0f,  40.0f }, { 170.0f, 240.0f,  50.0f },
};
static const float p99_blob[8][2] = {
    { 110.0f, 62.0f }, { -110.0f, 62.0f }, { 110.0f, -62.0f }, { -110.0f, -62.0f },
    { 0.0f, 96.0f }, { 0.0f, -96.0f }, { 140.0f, 0.0f }, { -140.0f, 0.0f },
};

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
            float dxs = ax - 64.0f; if (dxs < 0.0f) dxs = 0.0f;
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
    (void)sl; (void)seed;

    if (!p99_ready) p99_init();
    p99_tint(pal, tn);
    march = t * 0.18f; march -= 88.0f * floorf(march / 88.0f);
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

    for (i = 0; i < 8; i++) {
        bcol[i] = p99_pack(p99_band[i][0], p99_band[i][1], p99_band[i][2], tn);
        scol[i] = p99_pack(p99_band[i][0] * 0.35f, p99_band[i][1] * 0.35f,
                           p99_band[i][2] * 0.35f, tn);
    }
    court = p99_pack(14.0f, 24.0f, 90.0f, tn);
    dedge = p99_pack(10.0f, 10.0f, 40.0f, tn);

    for (y = 0; y < P99_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P99_LH);
        float dy = ly - 120.0f, ady = dy < 0.0f ? -dy : dy;
        const uint16_t *dr = p99_dist + (long)y * P99_LW;
        uint32_t *out = p99_low + (long)y * P99_LW;
        for (x = 0; x < P99_LW; x++) {
            float d = (float)dr[x] * 0.125f;
            float v = (d - march + 352.0f) * (1.0f / 11.0f) + hroll;
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
                const float aaw = 0.0164f;   /* one frame of march, in v */
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
            dc = p99_pack(p99_drum[di][0], p99_drum[di][1], p99_drum[di][2], tn);
            for (k = 0; k < 2; k++) {
                float x0 = k ? 8.0f : -56.0f, x1 = k ? 56.0f : -8.0f;
                int xa = (int)((160.0f + x0) * 2.0f), xb = (int)((160.0f + x1) * 2.0f);
                if (xa < 0) xa = 0; if (xb > P99_LW) xb = P99_LW;
                for (x = xa; x < xb; x++) {
                    float lx = ((float)x + 0.5f) * 0.5f, dx = lx - 160.0f;
                    float e0 = dx - x0, e1 = dx - x1;
                    if (e0 < 0.0f) e0 = -e0; if (e1 < 0.0f) e1 = -e1;
                    out[x] = (e0 < 1.5f || e1 < 1.5f) ? dedge : dc;
                }
            }
        }
        /* hot pink perimeter blobs */
        for (i = 0; i < 8; i++) {
            float by = p99_blob[i][1], bx = p99_blob[i][0];
            float ey = dy - by, q;
            uint32_t bc;
            int xa, xb;
            if (ey * ey >= 55.0f) continue;
            q = sqrtf(90.0f * (1.0f - ey * ey / 55.0f));
            {
                float pl = 0.7f + 0.3f * p99_lsin(t * 0.02f - 2.0f * P99_PI
                           * floorf(t * 0.02f / (2.0f * P99_PI)) + bx * 0.05f + by * 0.03f);
                bc = p99_pack(240.0f * pl, 60.0f * pl, 170.0f * pl, tn);
            }
            xa = (int)((160.0f + bx - q) * 2.0f); xb = (int)((160.0f + bx + q) * 2.0f);
            if (xa < 0) xa = 0; if (xb > P99_LW) xb = P99_LW;
            for (x = xa; x < xb; x++) out[x] = bc;
        }
    }
    p99_blit(fb, w, h);
}
