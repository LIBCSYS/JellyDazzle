/* 101 Physarum Veil — a slime mould building and rebuilding its network.
 * 7000 agents crawl over a chemical trail map. Each frame an agent samples the
 * map at three points ahead (left, centre, right), steers toward the strongest,
 * steps one pixel and deposits attractant; the map is then blurred and decayed.
 * That single rule — follow your own species' trail — is enough to grow the
 * transport networks Physarum polycephalum is famous for: the field starts as
 * a haze, condenses into travelling fronts, then resolves into veins, junctions
 * and closed cells that keep re-routing forever. No path is scripted and the
 * structure never repeats. Bright filaments on a black plate, so it stacks as
 * an overlay; hue is set by trail concentration, so trunk routes and the faint
 * exploratory haze read as different colour families. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p101_up;

#define P101_LW 320
#define P101_LH 240
#define P101_NA 7000
#define P101_SD 9.0f                  /* sensor distance, px  */
#define P101_SA 0.52f                 /* sensor angle, rad    */
#define P101_TA 0.36f                 /* turn per step, rad   */

static float p101_trail[P101_LW * P101_LH];
static float p101_tmp[P101_LW * P101_LH];
static uint8_t p101_img[P101_LW * P101_LH * 3];
static int *p101_xm;
static int p101_xm_w;
static float p101_ax[P101_NA], p101_ay[P101_NA], p101_ah[P101_NA];
static float p101_sin[4096];
static uint8_t p101_tone[2048];
static uint8_t p101_ramp[256][3];
static uint32_t p101_rs = 0x101B10AAu;
static int p101_ready;

static uint32_t p101_rnd(void)
{
    p101_rs ^= p101_rs << 13; p101_rs ^= p101_rs >> 17; p101_rs ^= p101_rs << 5;
    return p101_rs;
}
static float p101_s(float a)
{
    return p101_sin[((int)(a * 651.8986f + 0.5f)) & 4095];
}
static float p101_c(float a)
{
    return p101_sin[((int)(a * 651.8986f + 1024.5f)) & 4095];
}

static void p101_init(void)
{
    int i;
    for (i = 0; i < 4096; i++)
        p101_sin[i] = sinf((float)i * (6.2831853f / 4096.0f));
    for (i = 0; i < 2048; i++) {
        float v = 1.0f - expf(-(float)i * (4.6f / 2048.0f));
        p101_tone[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    for (i = 0; i < P101_NA; i++) {   /* scattered over the whole plate */
        p101_ax[i] = (float)(p101_rnd() >> 12 & 4095) * (P101_LW / 4096.0f);
        p101_ay[i] = (float)(p101_rnd() >> 12 & 4095) * (P101_LH / 4096.0f);
        p101_ah[i] = (float)(p101_rnd() >> 12 & 4095) * 0.0015340f;
    }
    p101_ready = 1;
}

static void p101_build_ramp(const uint32_t *pal, int base)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * 128) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {                 /* palettes contain near-black stretches */
            if (i) { p101_ramp[i][0] = p101_ramp[i-1][0];
                     p101_ramp[i][1] = p101_ramp[i-1][1];
                     p101_ramp[i][2] = p101_ramp[i-1][2]; }
            else   { p101_ramp[i][0] = p101_ramp[i][1] = p101_ramp[i][2] = 210; }
            continue;
        }
        p101_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p101_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p101_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static float p101_sample(float x, float y)
{
    int xi = (int)x, yi = (int)y;
    if (xi < 0) xi += P101_LW; else if (xi >= P101_LW) xi -= P101_LW;
    if (yi < 0) yi += P101_LH; else if (yi >= P101_LH) yi -= P101_LH;
    return p101_trail[yi * P101_LW + xi];
}

static void p101_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p101_xm_w != w) {
        free(p101_xm);
        p101_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p101_xm[x] = (int)(((long long)x * (P101_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p101_xm_w = w;
    }
    jd_up_blit(&p101_up, fb, w, h, p101_img, P101_LW, P101_LH);
}

void pattern_101(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    int i, x, y, hbase;
    (void)sl;

    if (!p101_ready) p101_init();
    hbase = (int)(t * 0.9f) + (int)(seed & 32767);
    p101_build_ramp(pal, hbase);

    /* A slice of the population is re-scattered every frame (whole colony
     * turned over in ~4.5 s). Without it the veins keep merging until only two
     * or three trunks are left; with it the mesh stays at a steady scale and
     * keeps re-routing indefinitely. */
    for (i = 0; i < 34; i++) {
        int k = (frame * 34 + i) % P101_NA;
        p101_ax[k] = (float)(p101_rnd() >> 12 & 4095) * (P101_LW / 4096.0f);
        p101_ay[k] = (float)(p101_rnd() >> 12 & 4095) * (P101_LH / 4096.0f);
        p101_ah[k] = (float)(p101_rnd() >> 12 & 4095) * 0.0015340f;
    }

    /* --- agents: sense three ways, steer, step, deposit --- */
    for (i = 0; i < P101_NA; i++) {
        float hd = p101_ah[i], px = p101_ax[i], py = p101_ay[i];
        float ch = p101_c(hd), sh = p101_s(hd);
        float fc = p101_sample(px + ch * P101_SD, py + sh * P101_SD);
        float cl = p101_c(hd - P101_SA), sll = p101_s(hd - P101_SA);
        float cr = p101_c(hd + P101_SA), sr = p101_s(hd + P101_SA);
        float fl = p101_sample(px + cl * P101_SD, py + sll * P101_SD);
        float fr = p101_sample(px + cr * P101_SD, py + sr * P101_SD);
        int xi, yi;
        if (fl > fc && fl > fr) hd -= P101_TA;
        else if (fr > fc && fr > fl) hd += P101_TA;
        else if (fc < fl && fc < fr)
            hd += ((p101_rnd() & 1) ? P101_TA : -P101_TA);
        hd += (float)((int)(p101_rnd() >> 22 & 255) - 128) * 0.00035f;
        px += p101_c(hd); py += p101_s(hd);
        if (px < 0.0f) px += P101_LW; else if (px >= P101_LW) px -= P101_LW;
        if (py < 0.0f) py += P101_LH; else if (py >= P101_LH) py -= P101_LH;
        p101_ah[i] = hd; p101_ax[i] = px; p101_ay[i] = py;
        xi = (int)px; yi = (int)py;
        if ((unsigned)xi < P101_LW && (unsigned)yi < P101_LH)
            p101_trail[yi * P101_LW + xi] += 1.0f;
    }

    /* --- diffuse (3x3 box on a torus) and decay --- */
    for (y = 0; y < P101_LH; y++) {
        const float *r0 = p101_trail + ((y + P101_LH - 1) % P101_LH) * P101_LW;
        const float *r1 = p101_trail + y * P101_LW;
        const float *r2 = p101_trail + ((y + 1) % P101_LH) * P101_LW;
        float *o = p101_tmp + y * P101_LW;
        for (x = 0; x < P101_LW; x++) {
            int xm = x ? x - 1 : P101_LW - 1, xp = x + 1 < P101_LW ? x + 1 : 0;
            o[x] = (r0[xm] + r0[x] + r0[xp] + r1[xm] + r1[x] + r1[xp]
                  + r2[xm] + r2[x] + r2[xp]) * (0.1111111f * 0.905f);
        }
    }
    memcpy(p101_trail, p101_tmp, sizeof p101_trail);

    /* --- shade --- */
    {
        int hoff = hbase / 18;
        for (y = 0; y < P101_LH; y++) {
            const float *r = p101_trail + y * P101_LW;
            uint8_t *row = p101_img + y * P101_LW * 3;
            for (x = 0; x < P101_LW; x++) {
                float v = r[x];
                int ti = (int)(v * 105.0f);
                int lum, hue, o = x * 3;
                const uint8_t *cp;
                if (ti > 2047) ti = 2047;
                lum = p101_tone[ti];
                hue = (hoff + (int)(v * 2.6f)) & 255;
                cp = p101_ramp[hue];
                row[o + 0] = (uint8_t)((cp[0] * lum) >> 8);
                row[o + 1] = (uint8_t)((cp[1] * lum) >> 8);
                row[o + 2] = (uint8_t)((cp[2] * lum) >> 8);
            }
        }
    }
    p101_blit(fb, w, h);
}
