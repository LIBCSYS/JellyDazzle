/* 095 Ring Machine — port of lab/patterns/095_ring_machine/proto.py
 * Two stacks of red concentric arcs bow outward like giant parentheses around
 * an "H" of blue->magenta gradient rectangles with a pulsing red crossbar and a
 * white diamond core; a red X-lattice of struts and dim green scanlines sit
 * behind. Full repaint at 640x480 (320x240 logical), bilinear upscale. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P95_LW 640
#define P95_LH 480
#define P95_N  (P95_LW * P95_LH)
#define P95_PI 3.14159265f

static uint32_t p95_low[P95_N];
static float    p95_sin[2048];
static int      p95_ready;

static void p95_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p95_sin[i] = sinf((float)i * (2.0f * P95_PI / 2048.0f));
    p95_ready = 1;
}

static inline float p95_lsin(float a)
{
    return p95_sin[((int)(a * 325.9493f + 2048.5f)) & 2047];
}

static void p95_tint(const uint32_t *pal, float *tn)
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

static void p95_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P95_LW << 16) / w);
    int fx0 = (int)(((long)P95_LW << 15) / w) - (1 << 15);
    int maxx = (P95_LW - 1) << 16, maxy = (P95_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P95_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P95_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p95_low + (long)y0 * P95_LW;
        r1 = p95_low + (long)y1 * P95_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P95_LW ? x0 + 1 : x0;
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

void pattern_095(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float tring, tscan, tn[3], bpul, rph;
    int x, y;
    (void)sl; (void)seed;

    if (!p95_ready) p95_init();
    p95_tint(pal, tn);
    tring = t * 0.02f;  tring -= 2.0f * P95_PI * floorf(tring / (2.0f * P95_PI));
    tscan = t * 0.01f;  tscan -= 2.0f * P95_PI * floorf(tscan / (2.0f * P95_PI));
    bpul  = 0.75f + 0.25f * p95_lsin(t * 0.015f - 2.0f * P95_PI
            * floorf(t * 0.015f / (2.0f * P95_PI)));
    rph   = p95_lsin(t * 0.008f - 2.0f * P95_PI
            * floorf(t * 0.008f / (2.0f * P95_PI))) * 0.8f;

    for (y = 0; y < P95_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P95_LH);
        float dy = ly - 120.0f, ay = dy < 0.0f ? -dy : dy;
        float dyc = ly - 120.0f, dy2 = dyc * dyc;
        int scan = (((int)ly) & 3) == 0;
        int inbar = ay < 11.0f;
        int rect = 0;
        float rr_c = 0.0f, bb_c = 0.0f;
        uint32_t *out = p95_low + (long)y * P95_LW;
        if (ly >= 36.0f && ly < 102.0f) {
            float u = (ly - 36.0f) / 66.0f;
            float s = p95_lsin(u * 2.4f + rph); if (s < 0.0f) s = 0.0f;
            rr_c = 90.0f + 150.0f * s; bb_c = 255.0f - 60.0f * s; rect = 1;
        } else if (ly >= 138.0f && ly < 204.0f) {
            float u = (ly - 138.0f) / 66.0f;
            float s = p95_lsin(u * 2.4f + rph); if (s < 0.0f) s = 0.0f;
            rr_c = 90.0f + 150.0f * s; bb_c = 255.0f - 60.0f * s; rect = 1;
        }
        for (x = 0; x < P95_LW; x++) {
            float lx = ((float)x + 0.5f) * (320.0f / (float)P95_LW);
            float dx = lx - 160.0f, ax = dx < 0.0f ? -dx : dx;
            float fr = 0.0f, fg = 0.0f, fb2 = 0.0f;
            int r, g, b;

            if (scan) fg = 55.0f + 25.0f * p95_lsin(lx * 0.05f + tscan);

            /* red X lattice */
            if ((ay - ax * 0.28f < 1.3428f && ax * 0.28f - ay < 1.3428f) ||
                (ay - ax * 0.55f < 1.2338f && ax * 0.55f - ay < 1.2338f) ||
                (ay - ax * 0.85f < 1.0705f && ax * 0.85f - ay < 1.0705f)) {
                fr = 150.0f; fg = 20.0f; fb2 = 30.0f;
            }

            /* red arc stacks: "(" left, ")" right */
            {
                float ex, rd, sv;
                ex = lx - 108.0f;
                if (-ex > 8.0f) {
                    rd = sqrtf(ex * ex + dy2);
                    if (rd > 26.0f && rd < 108.0f) {
                        sv = p95_lsin(rd * 0.55f - tring);
                        if (sv > 0.15f) {
                            float sh = 0.55f + 0.45f * p95_lsin(rd * 0.55f - tring + 1.3f);
                            fr = 255.0f * sh; fg = 30.0f * sh; fb2 = 45.0f * sh;
                        }
                    }
                }
                ex = lx - 212.0f;
                if (ex > 8.0f) {
                    rd = sqrtf(ex * ex + dy2);
                    if (rd > 26.0f && rd < 108.0f) {
                        sv = p95_lsin(rd * 0.55f - tring);
                        if (sv > 0.15f) {
                            float sh = 0.55f + 0.45f * p95_lsin(rd * 0.55f - tring + 1.3f);
                            fr = 255.0f * sh; fg = 30.0f * sh; fb2 = 45.0f * sh;
                        }
                    }
                }
            }

            if (lx >= 96.0f && lx < 224.0f) {
                if (rect) { fr = rr_c; fg = 30.0f; fb2 = bb_c; }
                if (inbar) { fr = 235.0f * bpul; fg = 25.0f; fb2 = 55.0f; }
            }
            if (ax + ay < 13.0f) {
                if (ax + ay < 6.0f) { fr = 255.0f; fg = 245.0f; fb2 = 250.0f; }
                else { fr = 255.0f; fg = 70.0f; fb2 = 90.0f; }
            }

            r = (int)(fr * tn[0]); g = (int)(fg * tn[1]); b = (int)(fb2 * tn[2]);
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            out[x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
    p95_blit(fb, w, h);
}
