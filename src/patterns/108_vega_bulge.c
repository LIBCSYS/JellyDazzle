/* 108 Vega Bulge — Vasarely op-art: a flat grid that swells into spheres.
 * Three invisible lenses drift under a ruled grid. Each displaces the lattice
 * radially by  d = A/(1 + r^2/R^2)  applied to (p - c), so the ruling stretches
 * near a lens centre and compresses at its rim; the eye reads the result as a
 * sphere pushing through the page (Vasarely's Vega series, done as light on
 * black instead of ink on white). Lines only — the interiors stay empty, so
 * this stacks over a ground without hiding it. Line brightness rides the local
 * stretch, which makes the bulges glow at their crowns, and hue walks with the
 * warped coordinate so each swell carries its own colour. Grid pitch, lens
 * strengths and lens positions all breathe on slow independent clocks. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p108_up;

#define P108_LW 480
#define P108_LH 360
#define P108_NB 3

static uint8_t p108_img[P108_LW * P108_LH * 3];
static int *p108_xm;
static int p108_xm_w;
static uint16_t p108_line[4096];      /* one grid period: the rule profile */
static uint16_t p108_fall[2048];      /* lens falloff 1/(1+s)             */
static uint8_t p108_ramp[256][3];
static int p108_ready;

static void p108_init(void)
{
    int i;
    for (i = 0; i < 4096; i++) {
        float s = (float)i * (1.0f / 4096.0f);
        float d = s < 0.5f ? s : 1.0f - s;       /* distance to the rule    */
        float v = 1.0f - d * 11.0f;
        if (v < 0.0f) v = 0.0f;
        p108_line[i] = (uint16_t)(v * v * 4095.0f);
    }
    for (i = 0; i < 2048; i++) {
        float s = (float)i * (7.0f / 2048.0f);
        float e = 1.0f - s * (1.0f / 7.0f);       /* compact support: the lens
                                                   * must reach exactly 0 or the
                                                   * grid steps at its rim */
        p108_fall[i] = (uint16_t)(4095.0f * e * e / (1.0f + s));
    }
    p108_ready = 1;
}

static void p108_build_ramp(const uint32_t *pal, int base)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * 128) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p108_ramp[i][0] = p108_ramp[i-1][0];
                     p108_ramp[i][1] = p108_ramp[i-1][1];
                     p108_ramp[i][2] = p108_ramp[i-1][2]; }
            else   { p108_ramp[i][0] = p108_ramp[i][1] = p108_ramp[i][2] = 210; }
            continue;
        }
        p108_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p108_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p108_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static void p108_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p108_xm_w != w) {
        free(p108_xm);
        p108_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p108_xm[x] = (int)(((long long)x * (P108_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p108_xm_w = w;
    }
    jd_up_blit(&p108_up, fb, w, h, p108_img, P108_LW, P108_LH);
}

void pattern_108(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float bx[P108_NB], by[P108_NB], ba[P108_NB], br[P108_NB];
    float pitch, ipitch, rot_c, rot_s;
    int x, y, i, hbase;
    (void)sl;

    if (!p108_ready) p108_init();
    {
        float ph = (float)(seed & 4095) * 0.00153f;
        float ang = t * 0.00042f + ph;
        for (i = 0; i < P108_NB; i++) {
            float a = t * (0.00051f + 0.00017f * (float)i) + ph + (float)i * 2.1f;
            float b = t * (0.00039f + 0.00013f * (float)i) + ph * 1.6f + (float)i * 1.3f;
            bx[i] = P108_LW * 0.5f + (78.0f + 30.0f * (float)i) * sinf(a);
            by[i] = P108_LH * 0.5f + (54.0f + 22.0f * (float)i) * sinf(b);
            ba[i] = (0.34f + 0.16f * sinf(t * 0.00067f + (float)i * 1.9f + ph))
                  * (i == 1 ? -1.0f : 1.0f);        /* one lens pinches */
            br[i] = 66.0f + 22.0f * (float)i
                  + 12.0f * sinf(t * 0.00058f + (float)i);
        }
        pitch = 25.0f + 4.0f * sinf(t * 0.00036f + ph);
        ipitch = 4096.0f / pitch;
        rot_c = cosf(ang * 0.35f); rot_s = sinf(ang * 0.35f);
        hbase = (int)(t * 1.5f) + (int)(seed & 32767);
    }
    p108_build_ramp(pal, hbase);

    for (y = 0; y < P108_LH; y++) {
        uint8_t *row = p108_img + y * P108_LW * 3;
        float fy = (float)y + 0.5f;
        for (x = 0; x < P108_LW; x++) {
            float fx = (float)x + 0.5f;
            float u = fx, v = fy, stretch = 0.0f;
            int gu, gv, li, lum, hue, o = x * 3;
            const uint8_t *cp;
            for (i = 0; i < P108_NB; i++) {
                float dx = fx - bx[i], dy = fy - by[i];
                float s = (dx * dx + dy * dy) / (br[i] * br[i]);
                int fi = (int)(s * 292.5f);          /* 2048 / 7 */
                float g;
                if (fi > 2047) continue;
                g = ba[i] * (float)p108_fall[fi] * (1.0f / 4096.0f);
                u += dx * g; v += dy * g;
                stretch += g < 0.0f ? -g : g;
            }
            {
                float ru = u * rot_c - v * rot_s;
                float rv = u * rot_s + v * rot_c;
                gu = p108_line[((int)(ru * ipitch)) & 4095];
                gv = p108_line[((int)(rv * ipitch)) & 4095];
                hue = (hbase / 22 + (int)(stretch * 150.0f)
                                  + (int)((ru + rv) * 0.12f)) & 255;
            }
            li = gu > gv ? gu : gv;
            li += ((gu < gv ? gu : gv) * 3) >> 3;    /* brighten crossings */
            lum = (li * (185 + (int)(stretch * 260.0f))) >> 12;
            if (lum > 255) lum = 255;
            cp = p108_ramp[hue];
            row[o + 0] = (uint8_t)((cp[0] * lum) >> 8);
            row[o + 1] = (uint8_t)((cp[1] * lum) >> 8);
            row[o + 2] = (uint8_t)((cp[2] * lum) >> 8);
        }
    }
    p108_blit(fb, w, h);
}
