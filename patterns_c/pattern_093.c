/* 093 Cathedral Fan — port of lab/patterns/093_cathedral_fan/proto.py
 * Red/pink ray fan pinched at a glowing horizon, stair-stepped blue spires with
 * green seams growing above and below it, golden concentric arcs breathing in
 * the four corners. Rendered into a 640x480 field (320x240 logical space) and
 * bilinearly upscaled. Restricted red/blue/green/gold palette, gently tinted by
 * the engine palette so it tracks the scheme crossfade. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P93_LW 640
#define P93_LH 480
#define P93_N  (P93_LW * P93_LH)
#define P93_PI 3.14159265f

static uint32_t p93_low[P93_N];
static float    p93_sin[2048];
static float    p93_pinch[257];
static int      p93_ready;

static const float p93_sp[3][4] = {
    {  36.0f, 15.0f, 96.0f, 0.0f },
    {  78.0f, 12.0f, 74.0f, 0.3f },
    { 116.0f,  9.0f, 52.0f, 0.6f },
};

static void p93_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p93_sin[i] = sinf((float)i * (2.0f * P93_PI / 2048.0f));
    for (i = 0; i <= 256; i++)
        p93_pinch[i] = powf(1.0f - (float)i / 256.0f, 2.2f);
    p93_ready = 1;
}

static inline float p93_lsin(float a)
{
    return p93_sin[((int)(a * 325.9493f + 2048.5f)) & 2047];
}

/* fast |atan2| for the mirrored quadrant, returns 0..pi/2 */
static inline float p93_ang(float ay, float ax)
{
    float a, s, r;
    if (ax < 1e-6f) return P93_PI * 0.5f;
    if (ay < 1e-6f) return 0.0f;
    a = (ax > ay) ? ay / ax : ax / ay;
    s = a * a;
    r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) r = 1.57079637f - r;
    return r;
}

/* Horizontal half of the bilinear blit for one source row, split out so each
 * source row is resampled once instead of once per output row it feeds (at
 * 480 -> 960 that is a 4x cut in horizontal work). Arithmetic is unchanged. */
static uint32_t *p93_hrb0, *p93_hg0, *p93_hrb1, *p93_hg1;
static int p93_hw;

static void p93_hrow(uint32_t *orb, uint32_t *og, const uint32_t *r,
                     int w, int fx0, int stepx, int maxx)
{
    int x, fx = fx0;
    for (x = 0; x < w; x++) {
        int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
        int x0 = cx >> 16, x1 = x0 + 1 < P93_LW ? x0 + 1 : x0;
        unsigned wx = (unsigned)((cx >> 8) & 255), sx = 256u - wx;
        uint32_t a = r[x0], b = r[x1];
        orb[x] = (((a & 0xFF00FFu) * sx + (b & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
        og[x]  = (((a & 0x00FF00u) * sx + (b & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
        fx += stepx;
    }
}

static void p93_blit(uint32_t *fb, int w, int h)
{
    int x, y, cy = -2;
    int stepx = (int)(((long)P93_LW << 16) / w);
    int fx0 = (int)(((long)P93_LW << 15) / w) - (1 << 15);
    int maxx = (P93_LW - 1) << 16, maxy = (P93_LH - 1) << 16;

    if (p93_hw != w) {
        free(p93_hrb0); free(p93_hg0); free(p93_hrb1); free(p93_hg1);
        p93_hrb0 = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)w);
        p93_hg0  = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)w);
        p93_hrb1 = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)w);
        p93_hg1  = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)w);
        if (!p93_hrb0 || !p93_hg0 || !p93_hrb1 || !p93_hg1) {
            free(p93_hrb0); free(p93_hg0); free(p93_hrb1); free(p93_hg1);
            p93_hrb0 = p93_hg0 = p93_hrb1 = p93_hg1 = 0; p93_hw = 0;
        } else p93_hw = w;
    }

    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P93_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * w;
        unsigned sy2;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P93_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p93_low + (long)y0 * P93_LW;
        r1 = p93_low + (long)y1 * P93_LW;
        sy2 = 256u - (unsigned)wy;

        if (!p93_hrb0) {                       /* alloc failed: direct path */
            for (x = 0; x < w; x++) {
                int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
                int x0 = cx >> 16, x1 = x0 + 1 < P93_LW ? x0 + 1 : x0;
                unsigned wx = (unsigned)((cx >> 8) & 255), sx = 256u - wx;
                uint32_t a = r0[x0], b = r0[x1], c = r1[x0], d = r1[x1];
                uint32_t trb = (((a & 0xFF00FFu)*sx + (b & 0xFF00FFu)*wx) >> 8) & 0xFF00FFu;
                uint32_t tg  = (((a & 0x00FF00u)*sx + (b & 0x00FF00u)*wx) >> 8) & 0x00FF00u;
                uint32_t brb = (((c & 0xFF00FFu)*sx + (d & 0xFF00FFu)*wx) >> 8) & 0xFF00FFu;
                uint32_t bg  = (((c & 0x00FF00u)*sx + (d & 0x00FF00u)*wx) >> 8) & 0x00FF00u;
                uint32_t orb = ((trb * sy2 + brb * (unsigned)wy) >> 8) & 0xFF00FFu;
                uint32_t og  = ((tg  * sy2 + bg  * (unsigned)wy) >> 8) & 0x00FF00u;
                dst[x] = 0xFF000000u | orb | og;
                fx += stepx;
            }
            continue;
        }

        if (y0 != cy) {
            if (y0 == cy + 1) {                /* the common step down a row */
                uint32_t *t;
                t = p93_hrb0; p93_hrb0 = p93_hrb1; p93_hrb1 = t;
                t = p93_hg0;  p93_hg0  = p93_hg1;  p93_hg1  = t;
            } else {
                p93_hrow(p93_hrb0, p93_hg0, r0, w, fx0, stepx, maxx);
            }
            p93_hrow(p93_hrb1, p93_hg1, r1, w, fx0, stepx, maxx);
            cy = y0;
        }
        for (x = 0; x < w; x++) {
            uint32_t orb = ((p93_hrb0[x] * sy2
                             + p93_hrb1[x] * (unsigned)wy) >> 8) & 0xFF00FFu;
            uint32_t og  = ((p93_hg0[x] * sy2
                             + p93_hg1[x] * (unsigned)wy) >> 8) & 0x00FF00u;
            dst[x] = 0xFF000000u | orb | og;
        }
    }
}

static void p93_tint(const uint32_t *pal, float *tn)
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

void pattern_093(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sweep, tarc, grow, tn[3];
    float hn[3], wk[3];
    int i, x, y;
    (void)seed;

    if (!p93_ready) p93_init();
    p93_tint(pal, tn);
    sweep = t * 0.006f;  sweep = sweep - 5.0f * floorf(sweep * 0.2f);
    tarc  = t * 0.03f;   tarc  = tarc - 2.0f * P93_PI * floorf(tarc * (1.0f / (2.0f * P93_PI)));
    grow  = (float)sl / 700.0f; if (grow > 1.0f) grow = 1.0f;

    for (i = 0; i < 3; i++) {
        float hmax = p93_sp[i][2], phz = p93_sp[i][3];
        float g = grow - phz; if (g < 0.0f) g = 0.0f;
        g = hmax * g / (1.0f - phz);
        hn[i] = 8.0f + g; if (hn[i] > hmax) hn[i] = hmax;
        wk[i] = p93_sp[i][1] * 9.0f / hmax;
    }

    for (y = 0; y < P93_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P93_LH);
        float dy = ly - 120.0f, ay = dy < 0.0f ? -dy : dy;
        float hor = expf(-ay * ay / 6.0f);
        float hr = 255.0f * hor, hg = 220.0f * hor, hb = 160.0f * hor;
        float ay2 = ay * ay;
        float step = floorf(ay * 0.1f);
        float seam = ay - 10.0f * step;
        uint32_t *out = p93_low + (long)y * P93_LW;
        float swid[3], sshd[3];
        int slive[3];
        for (i = 0; i < 3; i++) {
            swid[i] = p93_sp[i][1] - step * wk[i];
            sshd[i] = 1.0f - 0.55f * (ay / p93_sp[i][2]);
            if (sshd[i] < 0.0f) sshd[i] = 0.0f;
            slive[i] = (swid[i] > 0.0f) && (ay < hn[i]);
        }
        for (x = 0; x < P93_LW; x++) {
            float lx = ((float)x + 0.5f) * (320.0f / (float)P93_LW);
            float dx = lx - 160.0f, ax = dx < 0.0f ? -dx : dx;
            float ang = p93_ang(ay, ax);
            float u = ang * (2.0f / P93_PI);
            float q = u * 30.0f + sweep;
            float qf = q - floorf(q);
            float rr, gg, bb;
            int r, g, b;

            rr = hr; gg = hg; bb = hb;
            if (qf < 0.42f) {
                float rad = sqrtf(dx * dx + ay2);
                float dep = rad * (1.0f / 170.0f); if (dep > 1.0f) dep = 1.0f;
                float ri = p93_pinch[(int)(u * 256.0f) & 255] * (0.35f + 0.65f * dep);
                int gold = ((int)q % 5) == 0;
                rr += 255.0f * ri;
                gg += (gold ? 200.0f : 70.0f) * ri;
                bb += (gold ? 60.0f : 110.0f) * ri;
            }
            for (i = 0; i < 3; i++) {
                if (slive[i]) {
                    float d = ax - p93_sp[i][0]; if (d < 0.0f) d = -d;
                    if (d < swid[i]) {
                        if (seam < 2.0f) { rr = 60.0f; gg = 240.0f; bb = 90.0f; }
                        else { rr = 70.0f * sshd[i]; gg = 40.0f * sshd[i]; bb = 230.0f * sshd[i]; }
                    }
                }
            }
            {
                float cxd = 160.0f - ax, cyd = 120.0f - ay;
                float dc2 = cxd * cxd + cyd * cyd;
                if (dc2 < 78.0f * 78.0f) {
                    float dc = sqrtf(dc2);
                    if (p93_lsin(dc * 0.42f - tarc) > 0.55f) {
                        float af = 1.0f - dc * (1.0f / 78.0f);
                        rr = 240.0f * af + rr * 0.2f;
                        gg = 190.0f * af;
                        bb = 60.0f * af;
                    }
                }
            }
            r = (int)(rr * tn[0]); g = (int)(gg * tn[1]); b = (int)(bb * tn[2]);
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            out[x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
    p93_blit(fb, w, h);
}
