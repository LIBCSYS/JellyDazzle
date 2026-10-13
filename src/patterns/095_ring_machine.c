/* 095 Ring Machine — port of lab/patterns/095_ring_machine/proto.py
 * Two stacks of red concentric arcs bow outward like giant parentheses around
 * an "H" of blue->magenta gradient rectangles with a pulsing red crossbar and a
 * white diamond core; a red X-lattice of struts and dim green scanlines sit
 * behind. Full repaint at 640x480 (320x240 logical), bilinear upscale.
 * 3.6.5: the colours were a fixed red/blue/magenta/green set with a 15% tint
 * (dPal 3.4, C did nothing) and nothing read the seed (dSeed 0). Every role
 * now comes from a seed-chosen point on the ramp, and the seed also moves the
 * arc centres, ring pitch and direction, and the lattice slopes. */
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

static uint32_t p95_seed = 0xFFFFFFFFu;
static int   p95_poff, p95_pstep;
static float p95_acx, p95_rf, p95_rdir, p95_m[3], p95_thr[3];

static uint32_t p95_h(uint32_t x)
{
    x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16;
    return x;
}

static void p95_reseed(uint32_t seed)
{
    static const float m0[3] = { 0.28f, 0.55f, 0.85f };
    uint32_t a = p95_h(seed ^ 0x9595u), b = p95_h(a);
    float k = 0.80f + (float)((b >> 8) & 255u) * (0.45f / 255.0f);
    int i;
    p95_poff  = (int)(a & JD_PAL_MASK);
    p95_pstep = 5461 + (int)((b & 255u) * 10u);   /* roles 1/6..1/4.7 ramp apart */
    p95_acx   = 40.0f + (float)((a >> 15) & 31u);  /* arc centre offset from mid */
    p95_rf    = 0.42f + (float)((a >> 20) & 255u) * (0.30f / 255.0f);
    p95_rdir  = (a & 0x10000000u) ? 1.0f : -1.0f;
    for (i = 0; i < 3; i++) {
        /* keep each strut ~2.4 px thick whatever its slope */
        p95_m[i] = m0[i] * k;
        p95_thr[i] = 1.2f * sqrtf(1.0f + p95_m[i] * p95_m[i]);
    }
    p95_seed = seed;
}

/* One role colour from the ramp, brightest channel scaled to lv: a dark
 * stretch of the ramp cannot black a role out and the scene keeps its
 * original brightness. */
static void p95_role(const uint32_t *pal, int k, float lv, float *c)
{
    uint32_t u = pal[(p95_poff + k * p95_pstep) & JD_PAL_MASK];
    float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
    float b = (float)(u & 255), mx = r > g ? r : g;
    if (b > mx) mx = b;
    if (mx < 8.0f) { r = g = b = mx = 8.0f; }
    c[0] = r * lv / mx; c[1] = g * lv / mx; c[2] = b * lv / mx;
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
    float tring, tscan, bpul, rph;
    float carc[3], clat[3], cra[3], crb[3], cbar[3], cscn[3];
    float acl, acr;
    int x, y, i;
    (void)sl;

    if (!p95_ready) p95_init();
    if (seed != p95_seed) p95_reseed(seed);
    p95_role(pal, 0, 255.0f, carc);
    p95_role(pal, 1, 255.0f, cra);
    p95_role(pal, 2, 245.0f, crb);
    p95_role(pal, 3, 235.0f, cbar);
    p95_role(pal, 4, 80.0f, cscn);
    for (i = 0; i < 3; i++) clat[i] = carc[i] * 0.6f;
    acl = 160.0f - p95_acx; acr = 160.0f + p95_acx;
    tring = t * 0.02f * p95_rdir;  tring -= 2.0f * P95_PI * floorf(tring / (2.0f * P95_PI));
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
        float rs = 0.0f;
        uint32_t *out = p95_low + (long)y * P95_LW;
        if (ly >= 36.0f && ly < 102.0f) {
            float u = (ly - 36.0f) / 66.0f;
            float s = p95_lsin(u * 2.4f + rph); if (s < 0.0f) s = 0.0f;
            rs = s; rect = 1;
        } else if (ly >= 138.0f && ly < 204.0f) {
            float u = (ly - 138.0f) / 66.0f;
            float s = p95_lsin(u * 2.4f + rph); if (s < 0.0f) s = 0.0f;
            rs = s; rect = 1;
        }
        for (x = 0; x < P95_LW; x++) {
            float lx = ((float)x + 0.5f) * (320.0f / (float)P95_LW);
            float dx = lx - 160.0f, ax = dx < 0.0f ? -dx : dx;
            float fr = 0.0f, fg = 0.0f, fb2 = 0.0f;
            int r, g, b;

            if (scan) {
                float sv = (55.0f + 25.0f * p95_lsin(lx * 0.05f + tscan)) * (1.0f / 80.0f);
                fr = cscn[0] * sv; fg = cscn[1] * sv; fb2 = cscn[2] * sv;
            }

            /* X lattice */
            if ((ay - ax * p95_m[0] < p95_thr[0] && ax * p95_m[0] - ay < p95_thr[0]) ||
                (ay - ax * p95_m[1] < p95_thr[1] && ax * p95_m[1] - ay < p95_thr[1]) ||
                (ay - ax * p95_m[2] < p95_thr[2] && ax * p95_m[2] - ay < p95_thr[2])) {
                fr = clat[0]; fg = clat[1]; fb2 = clat[2];
            }

            /* red arc stacks: "(" left, ")" right */
            {
                float ex, rd, sv;
                ex = lx - acl;
                if (-ex > 8.0f) {
                    rd = sqrtf(ex * ex + dy2);
                    if (rd > 26.0f && rd < 108.0f) {
                        sv = p95_lsin(rd * p95_rf - tring);
                        if (sv > 0.15f) {
                            float sh = 0.55f + 0.45f * p95_lsin(rd * p95_rf - tring + 1.3f);
                            fr = carc[0] * sh; fg = carc[1] * sh; fb2 = carc[2] * sh;
                        }
                    }
                }
                ex = lx - acr;
                if (ex > 8.0f) {
                    rd = sqrtf(ex * ex + dy2);
                    if (rd > 26.0f && rd < 108.0f) {
                        sv = p95_lsin(rd * p95_rf - tring);
                        if (sv > 0.15f) {
                            float sh = 0.55f + 0.45f * p95_lsin(rd * p95_rf - tring + 1.3f);
                            fr = carc[0] * sh; fg = carc[1] * sh; fb2 = carc[2] * sh;
                        }
                    }
                }
            }

            if (lx >= 96.0f && lx < 224.0f) {
                if (rect) {
                    fr = cra[0] + (crb[0] - cra[0]) * rs;
                    fg = cra[1] + (crb[1] - cra[1]) * rs;
                    fb2 = cra[2] + (crb[2] - cra[2]) * rs;
                }
                if (inbar) { fr = cbar[0] * bpul; fg = cbar[1] * bpul; fb2 = cbar[2] * bpul; }
            }
            if (ax + ay < 13.0f) {
                if (ax + ay < 6.0f) { fr = 255.0f; fg = 245.0f; fb2 = 250.0f; }
                else { fr = 0.6f * cbar[0] + 100.0f; fg = 0.6f * cbar[1] + 100.0f;
                       fb2 = 0.6f * cbar[2] + 100.0f; }
            }

            r = (int)fr; g = (int)fg; b = (int)fb2;
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            out[x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
    p95_blit(fb, w, h);
}
