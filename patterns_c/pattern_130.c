/* 130 Pentagrid — de Bruijn's pentagrid, drawn as light instead of tiles.
 * Five families of parallel lines at 36-degree steps, each family a different
 * hue drawn from the palette, added together. The five-fold arrangement is the
 * dual of a Penrose tiling: it never repeats, so the crossing pattern wanders
 * forever without ever settling into a lattice. Triple crossings burn white.
 * Each family's offset gamma_i creeps at its own rate and the whole grid turns
 * about 1/100000 of a turn per frame, so the nodes drift like slow traffic.
 * Rendered with a per-family 32-bit phase accumulator: one add, one shift and
 * one table read per family per pixel, no trig and no division in the loop.
 * Overlay routine: black everywhere except the lines. */
#include "../jellydazzle.h"
#include <math.h>

#define P130_TAU 6.283185307179586f
#define P130_NF  5

static uint8_t  p130_lut[256];          /* phase position -> line intensity  */
static int      p130_ready;

static void p130_init(void)
{
    int i;
    for (i = 0; i < 256; i++) {
        float d = (float)i * (1.0f / 256.0f);   /* 0..1 through one period   */
        float e = d < 0.5f ? d : 1.0f - d;      /* distance to nearest line  */
        float v = 1.0f / (1.0f + (e * 74.0f) * (e * 74.0f));
        p130_lut[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    p130_ready = 1;
}

void pattern_130(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float spin, spacing, inv;
    uint32_t stepx[P130_NF], stepy[P130_NF], base[P130_NF];
    int cr[P130_NF], cg[P130_NF], cb[P130_NF], amp[P130_NF];
    int i, x, y, pbase;
    (void)sl; (void)seed;

    if (!p130_ready) p130_init();
    if (w < 2 || h < 2) return;

    spin    = t * 0.0000105f;
    spacing = (float)h / (5.6f + 0.9f * sinf(t * 0.00023f));
    inv     = 1.0f / spacing;
    pbase   = (int)(t * 17.0f);

    for (i = 0; i < P130_NF; i++) {
        float a  = spin + (float)i * (P130_TAU / (2.0f * P130_NF));
        float ca = cosf(a) * inv, sa = sinf(a) * inv;
        float g  = t * (0.00055f + 0.00021f * (float)i) + 0.7f * (float)i;
        uint32_t c = pal[(pbase + i * 6203) & JD_PAL_MASK];
        stepx[i] = (uint32_t)(int32_t)(ca * 4294967296.0f);
        stepy[i] = (uint32_t)(int32_t)(sa * 4294967296.0f);
        base[i]  = (uint32_t)(int32_t)((g - 0.5f * ((float)w * ca + (float)h * sa))
                                       * 4294967296.0f);
        amp[i]   = (int)(196.0f + 58.0f * sinf(t * 0.0013f + 1.7f * (float)i));
        cr[i] = (int)((c >> 16) & 255); cg[i] = (int)((c >> 8) & 255);
        cb[i] = (int)(c & 255);
        /* keep each family from going black when the palette dips */
        if (cr[i] + cg[i] + cb[i] < 150) { cr[i] += 50; cg[i] += 50; cb[i] += 50; }
    }

    for (y = 0; y < h; y++) {
        uint32_t ph[P130_NF];
        uint32_t *dst = fb + (long)y * (long)w;
        for (i = 0; i < P130_NF; i++)
            ph[i] = base[i] + stepy[i] * (uint32_t)y;
        for (x = 0; x < w; x++) {
            int r = 0, g = 0, b = 0, s = 0;
            for (i = 0; i < P130_NF; i++) {
                int iv = (int)p130_lut[ph[i] >> 24];
                ph[i] += stepx[i];
                if (iv > 2) {
                    iv = (iv * amp[i]) >> 8;
                    r += cr[i] * iv; g += cg[i] * iv; b += cb[i] * iv;
                    s += iv;
                }
            }
            if (r + g + b == 0) { dst[x] = 0xFF000000u; continue; }
            s -= 300;                               /* white-hot crossings   */
            s = s > 0 ? (s * s) >> 9 : 0;
            if (s > 220) s = 220;
            r = (r >> 8) + s; g = (g >> 8) + s; b = (b >> 8) + s;
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            dst[x] = 0xFF000000u | ((uint32_t)r << 16) |
                     ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}
