/* 128 Aggregate Bloom — diffusion-limited aggregation, grown twelve ways.
 * A real DLA cluster is built once at init: walkers are released on a circle
 * just outside the current cluster radius, take long jumps while far away and
 * single lattice steps once close, and freeze where they first touch. That
 * "stick on contact" rule is what produces the characteristic screening —
 * outer tips catch nearly every walker, so the cluster grows as a branched
 * dendrite with a fractal dimension near 1.7 rather than a disc.
 * One arm is grown, then stamped through six rotations and a mirror, so the
 * random branching reads as a crystal instead of a smudge. Playback is keyed to
 * the segment clock: particles appear in birth order over the first 1400
 * frames, the growing tips burning brighter than the settled interior, and the
 * whole bloom dissolves over the last 500 so the loop has no seam.
 * Overlay routine: a dendrite of light with a great deal of black around it. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P128_LW  640
#define P128_LH  480
#define P128_N   (P128_LW * P128_LH)
#define P128_G   321            /* odd, so there is a true centre cell      */
#define P128_C   160
#define P128_NP  2700
#define P128_SYM 6
#define P128_TAU 6.283185307179586f

static uint32_t p128_low[P128_N];
static uint32_t p128_ramp[256];
static int16_t  p128_px[P128_NP], p128_py[P128_NP];
static int      p128_np;
static uint8_t  p128_occ[P128_G * P128_G];
static int      p128_ready;

static uint32_t p128_rs = 0x1D0BEEF5u;
static uint32_t p128_rnd(void)
{
    p128_rs ^= p128_rs << 13; p128_rs ^= p128_rs >> 17; p128_rs ^= p128_rs << 5;
    return p128_rs;
}

static int p128_free(int x, int y)
{
    return (unsigned)x < P128_G && (unsigned)y < P128_G
        && !p128_occ[y * P128_G + x];
}

static void p128_init(void)
{
    static const int nx[4] = { 1, -1, 0, 0 }, ny[4] = { 0, 0, 1, -1 };
    float rmax = 2.0f;
    int i;
    p128_np = 0;
    for (i = 0; i < P128_G * P128_G; i++) p128_occ[i] = 0;
    p128_occ[P128_C * P128_G + P128_C] = 1;
    p128_px[p128_np] = 0; p128_py[p128_np] = 0; p128_np++;

    while (p128_np < P128_NP && rmax < 150.0f) {
        float a = -0.09f + (float)(p128_rnd() >> 8) * (0.67f / 16777216.0f);
        float fx = (rmax + 4.0f) * cosf(a), fy = (rmax + 4.0f) * sinf(a);
        int x = P128_C + (int)fx, y = P128_C + (int)fy, guard = 0, stuck = 0;
        while (guard++ < 6000) {
            float dx = (float)(x - P128_C), dy = (float)(y - P128_C);
            float r = sqrtf(dx * dx + dy * dy);
            int k;
            if (r > 2.4f * rmax + 40.0f) break;          /* wandered off     */
            if (r > rmax + 2.5f) {                       /* long jump inward */
                float j = r - rmax - 1.5f;
                float b = (float)(p128_rnd() >> 8) * (P128_TAU / 16777216.0f);
                if (j > 1.0f) {
                    x += (int)(j * cosf(b)); y += (int)(j * sinf(b));
                    continue;
                }
            }
            for (k = 0; k < 4; k++)
                if (!p128_free(x + nx[k], y + ny[k])) { stuck = 1; break; }
            if (stuck) break;
            k = (int)(p128_rnd() & 3);
            x += nx[k]; y += ny[k];
            if ((unsigned)x >= P128_G || (unsigned)y >= P128_G) break;
        }
        if (stuck && p128_free(x, y)) {
            float dx = (float)(x - P128_C), dy = (float)(y - P128_C);
            float r = sqrtf(dx * dx + dy * dy);
            /* the arm is confined to a 28-degree wedge, so the twelve stamped
             * copies interlock into a snowflake instead of overlapping */
            if (r > 3.0f && !(dx > 0.0f && dy >= 0.0f && dy <= 0.5317f * dx))
                continue;
            p128_occ[y * P128_G + x] = 1;
            p128_px[p128_np] = (int16_t)(x - P128_C);
            p128_py[p128_np] = (int16_t)(y - P128_C);
            p128_np++;
            if (r > rmax) rmax = r;
        }
    }
    p128_ready = 1;
}

static void p128_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P128_LW << 16) / w);
    int fx0 = (int)(((long)P128_LW << 15) / w) - (1 << 15);
    int maxx = (P128_LW - 1) << 16, maxy = (P128_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P128_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * (long)w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P128_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p128_low + (long)y0 * P128_LW;
        r1 = p128_low + (long)y1 * P128_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P128_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((cx >> 8) & 255), sx = 256u - wx;
            unsigned sy = 256u - (unsigned)wy;
            uint32_t a = r0[x0], b = r0[x1], c = r1[x0], d = r1[x1];
            uint32_t trb = (((a & 0xFF00FFu) * sx + (b & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t tg  = (((a & 0x00FF00u) * sx + (b & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t brb = (((c & 0xFF00FFu) * sx + (d & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t bg  = (((c & 0x00FF00u) * sx + (d & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t orb = ((trb * sy + brb * (unsigned)wy) >> 8) & 0xFF00FFu;
            uint32_t og  = ((tg  * sy + bg  * (unsigned)wy) >> 8) & 0x00FF00u;
            dst[x] = 0xFF000000u | orb | og;
            fx += stepx;
        }
    }
}

static void p128_blob(float fx, float fy, int r, int g, int b, int wq);

static void p128_splat(float fx, float fy, int r, int g, int b, int wq)
{
    int xi = (int)fx, yi = (int)fy, k;
    unsigned wx, wy2;
    if (xi < 1 || yi < 1 || xi >= P128_LW - 2 || yi >= P128_LH - 2) return;
    wx  = (unsigned)((fx - (float)xi) * 256.0f);
    wy2 = (unsigned)((fy - (float)yi) * 256.0f);
    for (k = 0; k < 4; k++) {
        unsigned kw = (k & 1 ? wx : 256u - wx) * (k & 2 ? wy2 : 256u - wy2);
        int q = (int)(((kw >> 8) * (unsigned)wq) >> 8);
        uint32_t *p = &p128_low[(yi + ((k >> 1) & 1)) * P128_LW + xi + (k & 1)];
        uint32_t v = *p;
        int rr = (int)((v >> 16) & 255) + ((r * q) >> 8);
        int gg = (int)((v >> 8) & 255) + ((g * q) >> 8);
        int bb = (int)(v & 255) + ((b * q) >> 8);
        if (rr > 255) rr = 255; if (gg > 255) gg = 255; if (bb > 255) bb = 255;
        *p = 0xFF000000u | ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb;
    }
}

/* a particle is a small soft bead, so the dendrite stays connected on screen */
static void p128_blob(float fx, float fy, int r, int g, int b, int wq)
{
    int h = (wq * 44) >> 8;
    p128_splat(fx, fy, r, g, b, wq);
    if (h < 3) return;
    p128_splat(fx - 1.05f, fy, r, g, b, h);
    p128_splat(fx + 1.05f, fy, r, g, b, h);
    p128_splat(fx, fy - 1.05f, r, g, b, h);
    p128_splat(fx, fy + 1.05f, r, g, b, h);
}

void pattern_128(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float grow, amp, scale, spin, cs[P128_SYM], sn[P128_SYM];
    float cx = P128_LW * 0.5f, cy = P128_LH * 0.5f;
    int i, k, pb, nvis;
    (void)seed;

    if (!p128_ready) p128_init();
    for (i = 0; i < P128_N; i++) p128_low[i] = 0xFF000000u;

    /* growth over the first 1400 frames, dissolve over the last 500 */
    grow = (float)sl * (1.0f / 1400.0f);
    if (grow > 1.0f) grow = 1.0f;
    grow = grow * grow * (3.0f - 2.0f * grow);
    amp = 1.0f;
    if (sl > 1548) {
        amp = 1.0f - (float)(sl - 1548) * (1.0f / 499.0f);
        if (amp < 0.0f) amp = 0.0f;
        amp = amp * amp * (3.0f - 2.0f * amp);
    }
    if (amp <= 0.002f) {                      /* fully dissolved: still opaque */
        long n = (long)w * (long)h;
        for (i = 0; i < (int)(n > 2147483000L ? 2147483000L : n); i++)
            fb[i] = 0xFF000000u;
        return;
    }
    nvis = (int)(grow * (float)p128_np);
    if (nvis < 1) nvis = 1;

    scale = (float)P128_LH * 0.00306f * (1.0f + 0.03f * sinf(t * 0.00042f));
    spin  = t * 0.00030f;
    for (k = 0; k < P128_SYM; k++) {
        float a = spin + (float)k * (P128_TAU / (float)P128_SYM);
        cs[k] = cosf(a) * scale; sn[k] = sinf(a) * scale;
    }

    pb = (int)(t * 3.0f);
    for (i = 0; i < 256; i++)
        p128_ramp[i] = pal[(pb + i * 128) & JD_PAL_MASK] & 0x00FFFFFFu;

    for (i = 0; i < nvis; i++) {
        float gx = (float)p128_px[i], gy = (float)p128_py[i];
        float age = (float)(nvis - i) * (1.0f / 340.0f);
        float hot = age < 1.0f ? (1.0f - age) : 0.0f;
        uint32_t col = p128_ramp[((i * 5) / 3 + (int)(t * 0.5f)) & 255];
        int cr = (int)((col >> 16) & 255), cg = (int)((col >> 8) & 255);
        int cb = (int)(col & 255), wq;
        int mxc = cr > cg ? cr : cg;
        if (cb > mxc) mxc = cb;
        if (mxc < 80) { cr += 80 - mxc; cg += 80 - mxc; cb += 80 - mxc; }
        cr += (int)(hot * 0.5f * (255 - cr)); cg += (int)(hot * 0.5f * (255 - cg));
        cb += (int)(hot * 0.5f * (255 - cb));
        wq = (int)((150.0f + 190.0f * hot) * amp);
        if (wq < 2) continue;
        for (k = 0; k < P128_SYM; k++) {
            float ux = gx * cs[k] - gy * sn[k];
            float uy = gx * sn[k] + gy * cs[k];
            p128_blob(cx + ux, cy + uy, cr, cg, cb, wq);
            p128_blob(cx + ux, cy - uy, cr, cg, cb, wq);
        }
    }
    p128_blit(fb, w, h);
}
