/* 092 Greek Key Panel — port of lab/patterns/092_greek_key_panel/proto.py
 * Frozen geometry, rolling DAC: a meander-tiled central panel inside a blue
 * wedge border with six-lobed rosettes and a horizontal ray fan behind it.
 * The class/shade map is built once into a 640x480 index buffer; each frame
 * only the 6x256 colour LUT is rewritten (the palette wheel rolls). */
#include "../jellydazzle.h"
#include <math.h>

#define P92_LW 640
#define P92_LH 480
#define P92_N  (P92_LW * P92_LH)
#define P92_PI 3.14159265f

static uint16_t p92_map[P92_N];        /* cls*256 + static shade index */
static uint32_t p92_lut[6 * 256];
static int      p92_ready;

static const unsigned char p92_key[12][13] = {
    "111111111111",
    "100000000000",
    "101111111101",
    "101000000101",
    "101011110101",
    "101010010101",
    "101010110101",
    "101010000101",
    "101011111101",
    "101000000001",
    "101111111111",
    "100000000000",
};

/* eight-stop looping wheel: yellow -> orange -> red -> magenta -> blue ->
 * azure -> cyan -> green (the palette supplies the actual hues) */
static float p92_stop[8][3];

static const float p92_off[6] = { 150.0f, 0.0f, 96.0f, 170.0f, 210.0f, 40.0f };
static const float p92_dim[6] = { 0.45f, 0.95f, 0.85f, 0.90f, 1.00f, 0.70f };

static void p92_build(void)
{
    int py, px, i, j;
    for (i = 0; i < P92_N; i++) p92_map[i] = 0;
    for (py = 0; py < P92_LH; py++) {
        float ly = ((float)py + 0.5f) * 0.5f;              /* 320x240 space */
        float dy = ly - 120.0f, ay = dy < 0 ? -dy : dy;
        for (px = 0; px < P92_LW; px++) {
            float lx = ((float)px + 0.5f) * 0.5f;
            float dx = lx - 160.0f, ax = dx < 0 ? -dx : dx;
            int cls = 0;
            float shade = 0.0f;
            float ang = atan2f(ay, ax + 0.001f);
            float q = ang * 40.0f / (P92_PI * 0.5f);
            if (ang < 0.42f && (q - floorf(q)) < 0.30f) { cls = 5; shade = ax * 0.5f; }
            if (ax < 132.0f && ay < 92.0f && !(ax < 116.0f && ay < 76.0f)) {
                float a2 = atan2f(dy, dx);
                float wq = a2 / (2.0f * P92_PI) * 24.0f;
                wq = wq - floorf(wq);
                if (wq < 0.5f) { cls = 3; shade = a2 * 18.0f; }
                else           { cls = 0; shade = 0.0f; }
            }
            if (ax < 116.0f && ay < 76.0f) {
                int kx = ((int)ax % 24) >> 1, ky = ((int)ay % 24) >> 1;
                cls = (p92_key[ky][kx] == '1') ? 2 : 1;
                shade = 0.0f;
            }
            for (i = 0; i < 5; i++) {
                static const float rx[5] = { -96.0f, -48.0f, 0.0f, 48.0f, 96.0f };
                for (j = 0; j < 2; j++) {
                    float ry = j ? 84.0f : -84.0f;
                    float ex = dx - rx[i], ey = dy - ry;
                    float rr = sqrtf(ex * ex + ey * ey);
                    if (rr < 12.0f) {
                        float ra = atan2f(ey, ex);
                        if (rr < 7.0f + 3.2f * cosf(6.0f * ra)) { cls = 4; shade = 0.0f; }
                    }
                }
            }
            {
                int s = (int)floorf(shade);
                s &= 255;
                p92_map[py * P92_LW + px] = (uint16_t)(cls * 256 + s);
            }
        }
    }
    p92_ready = 1;
}

static void p92_stops(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 8; i++) {
        uint32_t u = pal[((i * 4096) + 512) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g, mn = r < g ? r : g, k;
        if (b > mx) mx = b; if (b < mn) mn = b;
        if (mx < 24.0f) mx = 24.0f;
        k = (mn / mx) * 0.75f;
        p92_stop[i][0] = 255.0f * (r / mx - k) / (1.0f - k);
        p92_stop[i][1] = 255.0f * (g / mx - k) / (1.0f - k);
        p92_stop[i][2] = 255.0f * (b / mx - k) / (1.0f - k);
    }
}

static void p92_lutbuild(float roll)
{
    int cls, s;
    for (cls = 0; cls < 6; cls++) {
        float dim = p92_dim[cls];
        float base = p92_off[cls] + roll;
        for (s = 0; s < 256; s++) {
            float p = base + (float)s;
            int i0, i1, r, g, b;
            float f;
            p = p * (1.0f / 256.0f) * 8.0f;
            p = p - 8.0f * floorf(p * (1.0f / 8.0f));
            i0 = (int)p; if (i0 > 7) i0 = 7;
            i1 = (i0 + 1) & 7;
            f = p - (float)i0;
            r = (int)((p92_stop[i0][0] * (1.0f - f) + p92_stop[i1][0] * f) * dim);
            g = (int)((p92_stop[i0][1] * (1.0f - f) + p92_stop[i1][1] * f) * dim);
            b = (int)((p92_stop[i0][2] * (1.0f - f) + p92_stop[i1][2] * f) * dim);
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
            p92_lut[cls * 256 + s] = 0xFF000000u | ((uint32_t)r << 16)
                                   | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}

void pattern_092(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float roll = (float)frame * 0.35f;
    uint32_t stepx;
    int x, y;
    (void)sl; (void)seed;

    if (!p92_ready) p92_build();
    p92_stops(pal);
    roll = roll - 256.0f * floorf(roll * (1.0f / 256.0f));
    p92_lutbuild(roll);

    stepx = ((uint32_t)P92_LW << 16) / (uint32_t)w;
    for (y = 0; y < h; y++) {
        int sy = (int)(((long)y * P92_LH) / h);
        const uint16_t *src = p92_map + (long)sy * P92_LW;
        uint32_t *dst = fb + (long)y * w;
        uint32_t acc = 0;
        for (x = 0; x < w; x++) {
            dst[x] = p92_lut[src[acc >> 16]];
            acc += stepx;
        }
    }
}
