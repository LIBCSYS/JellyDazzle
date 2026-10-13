/* 096 Scanline Butterfly — port of lab/patterns/096_scanline_butterfly/proto.py
 * Full-field horizontal scanlines whose phase is warped by two wing lobes,
 * producing a green->yellow moire butterfly with a cyan diamond heart and a hot
 * red spindle; red/white ring ornaments in the corners. The wing/region/corner
 * maps are static (built once at 640x480); only the line phase crawls. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P96_LW 640
#define P96_LH 480
#define P96_N  (P96_LW * P96_LH)
#define P96_PI 3.14159265f

static uint32_t p96_low[P96_N];
static uint8_t  p96_wing[P96_N];
static uint8_t  p96_reg[P96_N];
static uint8_t  p96_dc[P96_N];
static float    p96_sin[2048];
static float    p96_wph[256];
static float    p96_ramp[256][3];
static int      p96_ready;
/* 3.6.5: it ignored its seed (every appearance the identical butterfly) and
 * drew from fixed RGB constants only tinted 15% by the palette, so C barely
 * moved it (variety audit dPal 4.7). Now the seed shapes the wings — lobe
 * spread, wing warp depth, line pitch, crawl direction, heart size — and
 * every colour, wings, heart, spindle and corner rings, is read from the
 * live palette at seed-chosen points on the ramp. */
static uint32_t p96_seed = 0xFFFFFFFFu;
static float    p96_lobe = 78.0f;       /* wing-lobe centre offset from axis */
static float    p96_wd   = 68.0f;       /* wing falloff distance             */
static float    p96_warp = 38.0f;       /* phase warp at the wing core       */
static float    p96_pitch = 0.85f;      /* scanline spatial frequency        */
static float    p96_dir  = 1.0f;        /* line crawl direction              */
static float    p96_dia  = 1.0f;        /* heart/diamond scale               */
static uint32_t p96_base;               /* where on the ramp the wings start */
static uint32_t p96_acc[4];             /* heart, spindle, ring A, ring B    */

static uint32_t p96_h(uint32_t x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }

/* Accents must read on black: a dark stretch of the ramp would draw the
 * heart nearly black. Keep the hue, lift the brightest channel to 190. */
static uint32_t p96_lift(uint32_t u)
{
    uint32_t r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
    uint32_t mx = r > g ? r : g; if (b > mx) mx = b;
    if (mx >= 190) return u & 0xFFFFFFu;
    if (mx < 8) return 0xC0C0C0u;
    r = r * 190 / mx; g = g * 190 / mx; b = b * 190 / mx;
    return (r << 16) | (g << 8) | b;
}

static inline float p96_lsin(float a)
{
    return p96_sin[((int)(a * 325.9493f + 2048.5f)) & 2047];
}

static void p96_init(void)
{
    int i, x, y;
    for (i = 0; i < 2048; i++)
        p96_sin[i] = sinf((float)i * (2.0f * P96_PI / 2048.0f));
    for (i = 0; i < 256; i++) {
        float wv = (float)i / 255.0f;
        p96_wph[i] = p96_warp * wv;
    }
    for (y = 0; y < P96_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P96_LH);
        float dy = ly - 120.0f, ay = dy < 0.0f ? -dy : dy;
        for (x = 0; x < P96_LW; x++) {
            float lx = ((float)x + 0.5f) * (320.0f / (float)P96_LW);
            float dx = lx - 160.0f, ax = dx < 0.0f ? -dx : dx;
            float ex = ax - p96_lobe, ey = dy * 1.15f;
            float dw = sqrtf(ex * ex + ey * ey);
            float wv = expf(-dw / p96_wd);
            float cxd = 160.0f - ax, cyd = 120.0f - ay;
            float dc = sqrtf(cxd * cxd + cyd * cyd);
            int idx = y * P96_LW + x, reg = 0;
            int wb = (int)(wv * 255.0f + 0.5f);
            if (wb > 255) wb = 255; if (wb < 0) wb = 0;
            p96_wing[idx] = (uint8_t)wb;
            if (ax / (46.0f * p96_dia) + ay / (62.0f * p96_dia) < 1.0f) reg = 1;
            {
                float sp = 1.0f - ay / (46.0f * p96_dia); if (sp < 0.0f) sp = 0.0f;
                if (ay < 46.0f * p96_dia && ax < 9.0f * sp) reg = 2;
            }
            if (dc < 46.0f) { reg = 3; p96_dc[idx] = (uint8_t)(dc * 5.0f); }
            else p96_dc[idx] = 0;
            p96_reg[idx] = (uint8_t)reg;
        }
    }
    p96_ready = 1;
}

/* Wing ramp: the wing weight walks half the palette from this run's base,
 * shaded from dim at the wing edge to full at the core, so the moire keeps
 * its dark-edge/bright-core read whatever the scheme. */
static void p96_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(p96_base + (uint32_t)i * 64u) & JD_PAL_MASK];
        float wv = (float)i / 255.0f, k = 0.45f + 0.55f * wv;
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 120.0f) k *= 120.0f / (mx < 8.0f ? 8.0f : mx);   /* no black wings */
        p96_ramp[i][0] = r * k; p96_ramp[i][1] = g * k; p96_ramp[i][2] = b * k;
    }
    for (i = 0; i < 4; i++)
        p96_acc[i] = p96_lift(pal[(p96_base + 16384u + (uint32_t)i * 4096u
                                   + (uint32_t)(i & 1) * 2048u) & JD_PAL_MASK]);
}

static void p96_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P96_LW << 16) / w);
    int fx0 = (int)(((long)P96_LW << 15) / w) - (1 << 15);
    int maxx = (P96_LW - 1) << 16, maxy = (P96_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P96_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P96_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p96_low + (long)y0 * P96_LW;
        r1 = p96_low + (long)y1 * P96_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P96_LW ? x0 + 1 : x0;
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

void pattern_096(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float tl, tc, ac[4][3];
    int x, y;
    (void)sl;

    if (seed != p96_seed || !p96_ready) {
        uint32_t a = p96_h(seed ^ 0x9696u), b = p96_h(a), c = p96_h(b), d = p96_h(c);
        p96_lobe  = 60.0f + (float)(a % 1000u) * 0.040f;          /* 60..100 */
        p96_wd    = 50.0f + (float)((a >> 12) % 1000u) * 0.036f;  /* 50..86  */
        p96_warp  = 26.0f + (float)(b % 1000u) * 0.026f;          /* 26..52  */
        p96_pitch = 0.62f + (float)((b >> 12) % 1000u) * 0.00056f;/* .62..1.18 */
        p96_dir   = (c & 1u) ? 1.0f : -1.0f;
        p96_dia   = 0.72f + (float)((c >> 8) % 1000u) * 0.00050f; /* .72..1.22 */
        p96_base  = d & JD_PAL_MASK;
        p96_seed  = seed;
        p96_init();                         /* wing/region maps for this run */
    }
    p96_buildpal(pal);
    for (x = 0; x < 4; x++) {
        ac[x][0] = (float)((p96_acc[x] >> 16) & 255);
        ac[x][1] = (float)((p96_acc[x] >> 8) & 255);
        ac[x][2] = (float)(p96_acc[x] & 255);
    }
    tl = p96_dir * t * 0.022f; tl -= 2.0f * P96_PI * floorf(tl / (2.0f * P96_PI));
    tc = t * 0.017f; tc -= 2.0f * P96_PI * floorf(tc / (2.0f * P96_PI));

    for (y = 0; y < P96_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P96_LH);
        float rowph = ly * p96_pitch + tl;
        const uint8_t *wg = p96_wing + (long)y * P96_LW;
        const uint8_t *rg = p96_reg + (long)y * P96_LW;
        const uint8_t *dcp = p96_dc + (long)y * P96_LW;
        uint32_t *out = p96_low + (long)y * P96_LW;
        for (x = 0; x < P96_LW; x++) {
            int wb = wg[x], reg = rg[x];
            float L = p96_lsin(rowph + p96_wph[wb]);
            int lit = L > -0.15f;
            float br = (L + 0.15f) * (1.0f / 1.15f);
            float fr, fg, fb2;
            int r, g, b;
            if (br < 0.0f) br = 0.0f; else if (br > 1.0f) br = 1.0f;

            if (lit) {
                fr = p96_ramp[wb][0] * br;
                fg = p96_ramp[wb][1] * br;
                fb2 = p96_ramp[wb][2] * br;
            } else {   /* the gap between lines: a faint shadow of the wing hue */
                fr = p96_ramp[wb][0] * 0.05f; fg = p96_ramp[wb][1] * 0.05f;
                fb2 = p96_ramp[wb][2] * 0.05f;
            }

            if (reg == 1) {
                float k = lit ? br * 1.2f : 0.06f;
                fr = ac[0][0] * k; fg = ac[0][1] * k; fb2 = ac[0][2] * k;
            } else if (reg == 2) {
                float k = lit ? (0.6f + 0.4f * br) * 1.3f : 0.3f;
                fr = ac[1][0] * k; fg = ac[1][1] * k; fb2 = ac[1][2] * k;
            } else if (reg == 3) {
                float rp = p96_lsin((float)dcp[x] * (0.55f / 5.0f) - tc);
                if (rp > 0.2f)        { fr = ac[2][0] * 1.2f; fg = ac[2][1] * 1.2f; fb2 = ac[2][2] * 1.2f; }
                else if (rp < -0.55f) { fr = 128.0f + ac[3][0] * 0.62f; fg = 128.0f + ac[3][1] * 0.62f;
                                        fb2 = 128.0f + ac[3][2] * 0.62f; }
            }

            r = (int)fr; g = (int)fg; b = (int)fb2;
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            out[x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
    p96_blit(fb, w, h);
}
