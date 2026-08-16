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
        p96_wph[i] = 38.0f * wv;
        p96_ramp[i][0] = wv < 0.5f ? wv * 2.0f * 190.0f : 190.0f + (wv - 0.5f) * 2.0f * 65.0f;
        p96_ramp[i][1] = 70.0f + 185.0f * (wv * 2.1f > 1.0f ? 1.0f : wv * 2.1f);
        p96_ramp[i][2] = wv > 0.75f ? (wv - 0.75f) * 4.0f * 200.0f : 20.0f;
    }
    for (y = 0; y < P96_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P96_LH);
        float dy = ly - 120.0f, ay = dy < 0.0f ? -dy : dy;
        for (x = 0; x < P96_LW; x++) {
            float lx = ((float)x + 0.5f) * (320.0f / (float)P96_LW);
            float dx = lx - 160.0f, ax = dx < 0.0f ? -dx : dx;
            float ex = ax - 78.0f, ey = dy * 1.15f;
            float dw = sqrtf(ex * ex + ey * ey);
            float wv = expf(-dw / 68.0f);
            float cxd = 160.0f - ax, cyd = 120.0f - ay;
            float dc = sqrtf(cxd * cxd + cyd * cyd);
            int idx = y * P96_LW + x, reg = 0;
            int wb = (int)(wv * 255.0f + 0.5f);
            if (wb > 255) wb = 255; if (wb < 0) wb = 0;
            p96_wing[idx] = (uint8_t)wb;
            if (ax / 46.0f + ay / 62.0f < 1.0f) reg = 1;
            {
                float sp = 1.0f - ay / 46.0f; if (sp < 0.0f) sp = 0.0f;
                if (ay < 46.0f && ax < 9.0f * sp) reg = 2;
            }
            if (dc < 46.0f) { reg = 3; p96_dc[idx] = (uint8_t)(dc * 5.0f); }
            else p96_dc[idx] = 0;
            p96_reg[idx] = (uint8_t)reg;
        }
    }
    p96_ready = 1;
}

static void p96_tint(const uint32_t *pal, float *tn)
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
    float tl, tc, tn[3];
    int x, y;
    (void)sl; (void)seed;

    if (!p96_ready) p96_init();
    p96_tint(pal, tn);
    tl = t * 0.022f; tl -= 2.0f * P96_PI * floorf(tl / (2.0f * P96_PI));
    tc = t * 0.017f; tc -= 2.0f * P96_PI * floorf(tc / (2.0f * P96_PI));

    for (y = 0; y < P96_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P96_LH);
        float rowph = ly * 0.85f + tl;
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
            } else { fr = 6.0f; fg = 14.0f; fb2 = 6.0f; }

            if (reg == 1) {
                if (lit) { fr = 40.0f * br; fg = 220.0f * br; fb2 = 235.0f * br; }
                else     { fr = 4.0f; fg = 10.0f; fb2 = 14.0f; }
            } else if (reg == 2) {
                fr = lit ? 255.0f * (0.6f + 0.4f * br) : 60.0f;
                fg = 30.0f * br; fb2 = 40.0f * br;
            } else if (reg == 3) {
                float rp = p96_lsin((float)dcp[x] * (0.55f / 5.0f) - tc);
                if (rp > 0.2f)        { fr = 235.0f; fg = 40.0f;  fb2 = 50.0f; }
                else if (rp < -0.55f) { fr = 245.0f; fg = 240.0f; fb2 = 225.0f; }
            }

            r = (int)(fr * tn[0]); g = (int)(fg * tn[1]); b = (int)(fb2 * tn[2]);
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            out[x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
    p96_blit(fb, w, h);
}
