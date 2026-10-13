/* 121 Chladni Sand — sand figures on a vibrating rectangular plate.
 * The nodal set of the standing wave
 *     f(u,v) = cos(n*pi*u)cos(m*pi*v) - cos(m*pi*u)cos(n*pi*v)
 * is where the plate does not move, so the sand piles up there. Modes n and m
 * are kept FRACTIONAL and drift continuously, so the curves re-lace themselves
 * instead of snapping between integer modes. The companion field
 *     g = cos(n*pi*u)cos(m*pi*v) + cos(m*pi*u)cos(n*pi*v)
 * costs nothing extra (same two products) and drives hue along each filament.
 * Both fields are separable: two per-column and two per-row cosine tables,
 * two multiplies per pixel, no trig and no division in the inner loop.
 * Overlay routine: everything except the thin sand lines is black. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P121_PI   3.14159265358979f
#define P121_MAXD 4096

static float    p121_ax[P121_MAXD], p121_bx[P121_MAXD];
static float    p121_cy[P121_MAXD], p121_dy[P121_MAXD];
static uint8_t  p121_lut[1024];              /* |f|*512 -> line intensity   */
static uint32_t p121_ramp[256];              /* g -> palette colour         */
static int      p121_ready;

static uint32_t p121_h(uint32_t x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }

static void p121_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float a = (float)i * (1.0f / 512.0f);   /* |f| */
        float v = 0.0022f / (0.0022f + a * a);
        p121_lut[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
    p121_ready = 1;
}

static void p121_buildramp(const uint32_t *pal, float off, int span)
{
    int i, base = (int)(off * 32768.0f);
    for (i = 0; i < 256; i++)
        p121_ramp[i] = pal[(base + i * span) & JD_PAL_MASK] & 0x00FFFFFFu;
}

void pattern_121(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float n, m, zoom, ushift, vshift, kn, km;
    float sn0, sm0, ph1, ph2, sz, roff;
    uint32_t a, b;
    int x, y, span;
    (void)sl;

    if (!p121_ready) p121_init();
    if (w > P121_MAXD || h > P121_MAXD || w < 2 || h < 2) return;

    /* 3.6.5: it ignored its seed, so every appearance began on the same plate
     * figure in the same hues. The seed now sets where on their glide the two
     * modes start and how high they sit, the plate zoom, the drift direction,
     * and the ramp start and span. All per-appearance constants, so the motion
     * is exactly as smooth as before (variety audit). */
    a = p121_h(seed ^ 0x1210u); b = p121_h(a);
    sn0  = 3.6f + (float)(a % 1000u) * 0.0016f;                /* 3.6..5.2   */
    sm0  = 6.6f + (float)((a >> 10) % 1000u) * 0.0022f;        /* 6.6..8.8   */
    ph1  = (float)((a >> 20) & 1023u) * (6.2831853f / 1024.0f);
    ph2  = (float)(b & 1023u) * (6.2831853f / 1024.0f);
    sz   = 0.86f + (float)((b >> 10) % 1000u) * 0.00030f;       /* .86..1.16  */
    roff = (float)((b >> 20) & 1023u) * (1.0f / 1024.0f);
    span = 64 + (int)(p121_h(b) % 96u);                         /* 64..159    */
    if (b & 0x80000000u) t = -t;              /* drift the other way round */

    /* fractional modes gliding through each other */
    n = sn0 + 2.10f * sinf(t * 0.00061f + ph1);
    m = sm0 + 3.10f * sinf(t * 0.00043f + ph2);
    zoom   = sz * (1.0f + 0.07f * sinf(t * 0.00035f + 0.7f));
    ushift = 0.035f * sinf(t * 0.00027f);
    vshift = 0.035f * sinf(t * 0.00031f + 2.4f);

    p121_buildramp(pal, roff + t * 0.000021f, span);

    kn = n * P121_PI;
    km = m * P121_PI;
    for (x = 0; x < w; x++) {
        float u = (((float)x + 0.5f) / (float)w - 0.5f) * zoom + 0.5f + ushift;
        p121_ax[x] = cosf(kn * u);
        p121_bx[x] = cosf(km * u);
    }
    for (y = 0; y < h; y++) {
        float v = (((float)y + 0.5f) / (float)h - 0.5f) * zoom + 0.5f + vshift;
        p121_cy[y] = cosf(kn * v);
        p121_dy[y] = cosf(km * v);
    }

    for (y = 0; y < h; y++) {
        float cy = p121_cy[y], dy = p121_dy[y];
        uint32_t *dst = fb + (long)y * (long)w;
        for (x = 0; x < w; x++) {
            float p = p121_ax[x] * dy;
            float q = p121_bx[x] * cy;
            float f = p - q;
            int   i = (int)((f < 0.0f ? -f : f) * 512.0f);
            int   iv, gi, hot, r, g, b;
            uint32_t c;
            if (i > 1023) { dst[x] = 0xFF000000u; continue; }
            iv = p121_lut[i];
            if (iv < 3) { dst[x] = 0xFF000000u; continue; }
            gi  = (int)((p + q + 2.0f) * 63.0f) & 255;
            c   = p121_ramp[gi];
            hot = (iv * iv) >> 10;
            r = (int)(((c >> 16) & 255) * iv >> 8) + hot;
            g = (int)(((c >> 8) & 255) * iv >> 8) + hot;
            b = (int)((c & 255) * iv >> 8) + hot;
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            dst[x] = 0xFF000000u | ((uint32_t)r << 16) |
                     ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}
