/* 127 Dipole Filings — iron filings around three drifting magnets.
 * Three 2-D magnetic dipoles orbit slowly while their moment vectors turn at
 * their own rates. The superposed field at any point is
 *     B = sum_k [ 2 (m_k . r_k) r_k / |r_k|^2  -  m_k ] / |r_k|^2
 * and a field line is just that field integrated: seed 28 points on a small
 * circle around each magnet and walk 340 unit steps along B, laying down a
 * glowing trace as you go. Where two magnets oppose, the lines bend away from a
 * neutral point and the whole bundle re-routes; where they attract, the lines
 * arch across and join. Because the magnets move slowly and the field is a
 * smooth function of their positions, the entire filing pattern flows.
 * Hue follows arclength, so each strand runs through the palette from its
 * source outward, and brightness falls off with distance travelled.
 * Overlay routine: strands of light on black, nothing filled. */
#include "../jellydazzle.h"
#include <math.h>

#define P127_LW  640
#define P127_LH  480
#define P127_N   (P127_LW * P127_LH)
#define P127_ND  3
#define P127_NS  22
#define P127_NR  4
#define P127_STP 340
#define P127_TAU 6.283185307179586f

static uint32_t p127_low[P127_N];
static uint32_t p127_ramp[256];

static void p127_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P127_LW << 16) / w);
    int fx0 = (int)(((long)P127_LW << 15) / w) - (1 << 15);
    int maxx = (P127_LW - 1) << 16, maxy = (P127_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P127_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * (long)w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P127_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p127_low + (long)y0 * P127_LW;
        r1 = p127_low + (long)y1 * P127_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P127_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((cx >> 8) & 255), sx = 256u - wx;
            unsigned sy = 256u - (unsigned)wy;
            uint32_t a = r0[x0], b = r0[x1], c = r1[x0], d = r1[x1];
            uint32_t trb = (((a & 0xFF00FFu) * sx + (b & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t tg  = (((a & 0x00FF00u) * sx + (b & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t brb = (((c & 0xFF00FFu) * sx + (d & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t bg  = (((c & 0x00FF00u) * sx + (d & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t orb = ((trb * sy + brb * (unsigned)wy) >> 8) & 0xFF00FFu;
            uint32_t og  = ((tg  * sy + bg  * (unsigned)wy) >> 8) & 0x00FF00u;
            dst[x] = 0xFF000000u | orb | og;
            fx += stepx;
        }
    }
}

static void p127_splat(float fx, float fy, int r, int g, int b, int wq)
{
    int xi = (int)fx, yi = (int)fy, k;
    unsigned wx, wy2;
    if (xi < 1 || yi < 1 || xi >= P127_LW - 2 || yi >= P127_LH - 2) return;
    wx  = (unsigned)((fx - (float)xi) * 256.0f);
    wy2 = (unsigned)((fy - (float)yi) * 256.0f);
    for (k = 0; k < 4; k++) {
        unsigned kw = (k & 1 ? wx : 256u - wx) * (k & 2 ? wy2 : 256u - wy2);
        int q = (int)(((kw >> 8) * (unsigned)wq) >> 8);
        uint32_t *p = &p127_low[(yi + ((k >> 1) & 1)) * P127_LW + xi + (k & 1)];
        uint32_t v = *p;
        int rr = (int)((v >> 16) & 255) + ((r * q) >> 8);
        int gg = (int)((v >> 8) & 255) + ((g * q) >> 8);
        int bb = (int)(v & 255) + ((b * q) >> 8);
        if (rr > 255) rr = 255; if (gg > 255) gg = 255; if (bb > 255) bb = 255;
        *p = 0xFF000000u | ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb;
    }
}

void pattern_127(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float dx_[P127_ND], dy_[P127_ND], mx_[P127_ND], my_[P127_ND];
    float cx = P127_LW * 0.5f, cy = P127_LH * 0.5f;
    int i, k, s, pb;
    (void)sl; (void)seed;

    for (i = 0; i < P127_N; i++) p127_low[i] = 0xFF000000u;

    pb = (int)(t * 4.0f);
    for (i = 0; i < 256; i++)
        p127_ramp[i] = pal[(pb + i * 128) & JD_PAL_MASK] & 0x00FFFFFFu;

    for (k = 0; k < P127_ND; k++) {
        float a  = t * 0.00038f + (float)k * (P127_TAU / P127_ND);
        float rr = 128.0f + 22.0f * sinf(t * 0.00029f + (float)k * 1.7f);
        float ma = t * 0.00051f * (k == 1 ? -1.0f : 1.0f) + (float)k * 2.1f;
        float st = (k == 1 ? -1.0f : 1.0f) * (1.0f + 0.25f * sinf(t * 0.00043f + (float)k));
        dx_[k] = cx + rr * cosf(a);
        dy_[k] = cy + rr * sinf(a) * 0.78f;
        mx_[k] = cosf(ma) * st * 900.0f;
        my_[k] = sinf(ma) * st * 900.0f;
    }

    for (k = 0; k < P127_ND; k++) {
      int ri;
      for (ri = 0; ri < P127_NR; ri++) {
        static const float seedr[P127_NR] = { 5.5f, 12.0f, 26.0f, 55.0f };
        for (s = 0; s < P127_NS; s++) {
            float a = (float)s * (P127_TAU / (float)P127_NS)
                    + 0.21f * (float)k + 0.13f * (float)ri;
            float px = dx_[k] + seedr[ri] * cosf(a);
            float py = dy_[k] + seedr[ri] * sinf(a);
            int dir = (s & 1) ? 1 : -1;
            int st;
            for (st = 0; st < P127_STP; st++) {
                float bx = 0.0f, by = 0.0f, mag, il;
                int j, cr, cg, cb, wq;
                uint32_t col;
                for (j = 0; j < P127_ND; j++) {
                    float rx = px - dx_[j], ry = py - dy_[j];
                    float r2 = rx * rx + ry * ry + 9.0f;
                    float iv = 1.0f / r2;
                    float sdot = 2.0f * (mx_[j] * rx + my_[j] * ry) * iv;
                    bx += (sdot * rx - mx_[j]) * iv;
                    by += (sdot * ry - my_[j]) * iv;
                }
                mag = bx * bx + by * by;
                if (mag < 1e-9f) break;
                il = 1.0f / sqrtf(mag);
                px += bx * il * 1.45f * (float)dir;
                py += by * il * 1.45f * (float)dir;
                if (px < 1.0f || py < 1.0f || px > P127_LW - 2.0f || py > P127_LH - 2.0f)
                    break;
                col = p127_ramp[((st * 3) / 2 + k * 74 + (int)(t * 0.6f)) & 255];
                cr = (int)((col >> 16) & 255); cg = (int)((col >> 8) & 255);
                cb = (int)(col & 255);
                {
                    int mxc = cr > cg ? cr : cg; if (cb > mxc) mxc = cb;
                    if (mxc < 70) { cr += 70 - mxc; cg += 70 - mxc; cb += 70 - mxc; }
                }
                wq = 430 - (st * 330) / P127_STP;
                p127_splat(px, py, cr, cg, cb, wq);
                if (st > 6) {
                    for (j = 0; j < P127_ND; j++) {
                        float rx = px - dx_[j], ry = py - dy_[j];
                        if (rx * rx + ry * ry < 16.0f) { st = P127_STP; break; }
                    }
                }
            }
        }
      }
    }
    p127_blit(fb, w, h);
}
