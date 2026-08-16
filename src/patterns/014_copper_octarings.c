/* pattern_014 — copper octarings: Amiga copper bars promoted from scanlines to
 * radius, drawn as crisp concentric octagons (octagonal norm) on midnight blue,
 * additively layered where they cross. The octagon frame rotates very slowly.
 * Port of lab/patterns/014_copper_octarings/proto.py. Repaint pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define CL 1152                    /* colour-line entries, 1/4 proto unit each */

static float prof[256];            /* clip(1-d/17)^1.5 bar profile           */
static int ready = 0;

static void setup(void) {
    for (int i = 0; i < 256; i++) {
        float x = 1.0f - (float)i / 255.0f;
        prof[i] = powf(x, 1.5f);
    }
}

/* pull a palette colour and push it to neon (max channel = 255) */
static void neon(const uint32_t *pal, int idx, float *r, float *g, float *b) {
    uint32_t c = pal[idx & JD_PAL_MASK];
    float cr = (float)((c >> 16) & 255), cg = (float)((c >> 8) & 255),
          cb = (float)(c & 255);
    float m = cr > cg ? cr : cg; if (cb > m) m = cb;
    float k = (m > 1.0f) ? (255.0f / m) : 1.0f;
    *r = cr * k; *g = cg * k; *b = cb * k;
}

void pattern_014(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!ready) { setup(); ready = 1; }
    const float t = (float)frame;

    /* ---- build this frame's colour line over octagonal radius ---- */
    static const float bw[6]  = { 0.0032f, 0.0026f, 0.0037f, 0.0028f,
                                  0.0023f, 0.0034f };
    static const float bph[6] = { 0.0f, 1.1f, 2.3f, 3.4f, 4.6f, 5.5f };
    float br[6], bg[6], bb[6], pos[6];
    for (int k = 0; k < 6; k++) {
        neon(pal, (int)(seed & 0x7FFFu) + k * 5461 + (int)(t * 0.05f),
             &br[k], &bg[k], &bb[k]);
        pos[k] = 96.0f + 76.0f * sinf(t * bw[k] + bph[k]);
    }

    uint32_t line[CL];
    for (int r = 0; r < CL; r++) {
        float fr = (float)r * 0.25f;
        float cr = 10.0f, cg = 6.0f;
        float cb = 30.0f + 16.0f * sinf(fr * 0.02f - t * 0.005f);
        for (int k = 0; k < 6; k++) {
            float d = fabsf(fr - pos[k]);
            if (d >= 17.0f) continue;
            float wg = prof[(int)(d * (255.0f / 17.0f))];
            cr += wg * br[k]; cg += wg * bg[k]; cb += wg * bb[k];
        }
        int ir = (int)cr, ig = (int)cg, ib = (int)cb;
        if (ir > 255) ir = 255; if (ig > 255) ig = 255; if (ib > 255) ib = 255;
        line[r] = 0xFF000000u | ((uint32_t)ir << 16) | ((uint32_t)ig << 8)
                | (uint32_t)ib;
    }

    /* ---- fill: one octagonal-norm lookup per pixel ---- */
    const float sc = 320.0f / (float)w;          /* screen px -> proto units */
    const float th = t * 0.0015f;
    const float co = cosf(th), si = sinf(th);
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;

    for (int y = 0; y < h; y++) {
        float dy = ((float)y - cy) * sc;
        uint32_t *row = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            float dx = ((float)x - cx) * sc;
            float rx = dx * co - dy * si;
            float ry = dx * si + dy * co;
            float ax = fabsf(rx), ay = fabsf(ry);
            float r8 = ax > ay ? ax : ay;
            float dg = (ax + ay) * 0.70710678f;
            if (dg > r8) r8 = dg;
            int i = (int)(r8 * 4.0f);
            if (i >= CL) i = CL - 1;
            row[x] = line[i];
        }
    }
}
