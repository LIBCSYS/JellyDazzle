/* 089 Oval Drums — full-screen concentric racetrack (stadium) rings in rolling
 * rainbow bands around two white-framed striped drums, plum blobs pulsing on
 * the perimeter. Port of lab/patterns/089_oval_drums/proto.py.
 * Repaint pattern: the ring field is a 1-D LUT over the stadium distance, the
 * drums are rectangle fills driven by a per-row ramp LUT. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P89_TAU 6.28318530718f
#define P89_HSPAN 410.0f

static float p89_ptab[1024][3];
static float p89_sin[2048];
static uint8_t p89_ring[1024][3];
static int p89_inited;

static const float p89_bx[3] = { 122.0f,  0.0f, 42.0f };
static const float p89_by[3] = {   0.0f, 58.0f, 52.0f };

static void p89_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p89_sin[i] = sinf((float)i * (P89_TAU / 2048.0f));
    p89_inited = 1;
}

static void p89_buildpal(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 24.0f) mx = 24.0f;
        p89_ptab[i][0] = r / mx; p89_ptab[i][1] = g / mx; p89_ptab[i][2] = b / mx;
    }
}

static float p89_lsin(float a)
{
    return p89_sin[((int)(a * 325.949318f + 32768.5f)) & 2047];
}

void pattern_089(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sx = (float)w / 320.0f, sy = (float)h / 240.0f;
    float isx = 1.0f / sx, isy = 1.0f / sy;
    float rsh = t * 0.008f, hdrift = t * 0.0012f;
    int i, x, y, s1, s2;
    (void)sl; (void)seed;

    if (!p89_inited) p89_init();
    p89_buildpal(pal);

    /* ---- stadium ring LUT over d in [0,256) ---- */
    for (i = 0; i < 1024; i++) {
        float d = (float)i * 0.25f;
        float ph = d * (1.0f / 15.0f) - rsh;
        float fl = floorf(ph), fr = ph - fl;
        float hue = fl * 0.13f + hdrift;
        float bri = 0.35f + 0.60f * p89_lsin(fr * 3.14159265f);
        const float *c = p89_ptab[(int)(hue * P89_HSPAN + 65536.0f) & 1023];
        int k;
        for (k = 0; k < 3; k++) {
            int q = (int)(c[k] * bri * 255.0f);
            if (q > 255) q = 255; if (q < 0) q = 0;
            p89_ring[i][k] = (uint8_t)q;
        }
    }
    for (y = 0; y < h; y++) {
        float py = ((float)y + 0.5f) * isy - 120.0f;
        float qy = py * py;
        uint32_t *row = fb + (long)y * w;
        for (x = 0; x < w; x++) {
            float ax = fabsf(((float)x + 0.5f) * isx - 160.0f) - 70.0f;
            float d;
            int k;
            if (ax < 0.0f) ax = 0.0f;
            d = sqrtf(qy + ax * ax);
            k = (int)(d * 4.0f); if (k > 1023) k = 1023;
            row[x] = 0xFF000000u | ((uint32_t)p89_ring[k][0] << 16)
                   | ((uint32_t)p89_ring[k][1] << 8) | (uint32_t)p89_ring[k][2];
        }
    }

    /* ---- perimeter plum blobs (mirrored) ---- */
    for (i = 0; i < 3; i++) {
        float amp = 0.7f + 0.3f * p89_lsin(t * 0.013f + p89_bx[i]);
        for (s1 = -1; s1 <= 1; s1 += 2) {
            if (p89_bx[i] == 0.0f && s1 > 0) continue;
            for (s2 = -1; s2 <= 1; s2 += 2) {
                float ccx, ccy;
                int x0, x1, y0, y1;
                if (p89_by[i] == 0.0f && s2 > 0) continue;
                ccx = 160.0f + (float)s1 * p89_bx[i];
                ccy = 120.0f + (float)s2 * p89_by[i];
                x0 = (int)((ccx - 10.0f) * sx); x1 = (int)((ccx + 10.0f) * sx) + 1;
                y0 = (int)((ccy - 10.0f) * sy); y1 = (int)((ccy + 10.0f) * sy) + 1;
                if (x0 < 0) x0 = 0; if (x1 > w) x1 = w;
                if (y0 < 0) y0 = 0; if (y1 > h) y1 = h;
                for (y = y0; y < y1; y++) {
                    float dy = ((float)y + 0.5f) * isy - ccy;
                    float qy = dy * dy;
                    uint32_t *row = fb + (long)y * w;
                    for (x = x0; x < x1; x++) {
                        float dx = ((float)x + 0.5f) * isx - ccx;
                        float bl = 1.0f - sqrtf(qy + dx * dx) * 0.1f;
                        uint32_t p;
                        int ir, ig, ib;
                        if (bl <= 0.0f) continue;
                        bl *= amp;
                        p = row[x];
                        ir = (int)((p >> 16) & 255) + (int)(bl * 0.35f * 255.0f);
                        ig = (int)((p >> 8) & 255) + (int)(bl * 0.10f * 255.0f);
                        ib = (int)(p & 255) + (int)(bl * 0.35f * 255.0f);
                        if (ir > 255) ir = 255; if (ig > 255) ig = 255;
                        if (ib > 255) ib = 255;
                        row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                               | ((uint32_t)ig << 8) | (uint32_t)ib;
                    }
                }
            }
        }
    }

    /* ---- two striped drums with warm-white outlines ---- */
    for (s1 = -1; s1 <= 1; s1 += 2) {
        float dcx = 160.0f + (float)s1 * 38.0f;
        int x0 = (int)((dcx - 33.0f) * sx), x1 = (int)((dcx + 33.0f) * sx) + 1;
        int y0 = (int)((120.0f - 23.5f) * sy), y1 = (int)((120.0f + 23.5f) * sy) + 1;
        if (x0 < 0) x0 = 0; if (x1 > w) x1 = w;
        if (y0 < 0) y0 = 0; if (y1 > h) y1 = h;
        for (y = y0; y < y1; y++) {
            float py = ((float)y + 0.5f) * isy - 120.0f;
            float ay = fabsf(py);
            float g = py * (1.0f / 12.0f) - t * 0.016f * (float)s1;
            float dh, bri, dr, dg, db;
            const float *c;
            uint32_t *row = fb + (long)y * w;
            int oy = (fabsf(ay - 21.0f) < 1.8f);
            g -= floorf(g);
            dh = 0.32f - 0.30f * g + hdrift;
            bri = 0.35f + 0.65f * (0.5f + 0.5f * p89_lsin(g * P89_TAU));
            c = p89_ptab[(int)(dh * P89_HSPAN + 65536.0f) & 1023];
            dr = c[0] * bri; dg = c[1] * bri; db = c[2] * bri;
            for (x = x0; x < x1; x++) {
                float dx = ((float)x + 0.5f) * isx - dcx;
                float ax = fabsf(dx);
                float cr, cg, cb;
                int ir, ig, ib;
                if ((fabsf(ax - 31.0f) < 1.8f && ay < 23.0f)
                    || (oy && ax < 32.5f)) {
                    cr = 0.95f; cg = 0.92f; cb = 0.80f;
                } else if (ax < 31.0f && ay < 21.0f) {
                    cr = dr; cg = dg; cb = db;
                } else continue;
                ir = (int)(cr * 255.0f); if (ir > 255) ir = 255;
                ig = (int)(cg * 255.0f); if (ig > 255) ig = 255;
                ib = (int)(cb * 255.0f); if (ib > 255) ib = 255;
                row[x] = 0xFF000000u | ((uint32_t)ir << 16)
                       | ((uint32_t)ig << 8) | (uint32_t)ib;
            }
        }
    }
}
