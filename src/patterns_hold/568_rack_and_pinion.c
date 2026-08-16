/* 568 Rack and Pinion — a toothed rack slides left and right on a slow
 * sine (eases to a stop, reverses) and a pinion above it rolls along,
 * phase locked to the rack so teeth interlock at every instant; a second
 * gear rides on the pinion.  Rack teeth each own a drifting hue and swap
 * colour with the pinion's teeth as they engage.  Figure overlay. */
#include "_fig541.h"

#define NR568 96
static gk g568;
static fg_gear pin568, top568;
static float rc568[NR568][3];
static uint32_t s568 = 0xFFFFFFFFu;

void pattern_568(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g568, w, h);
    gk_clear(&g568);
    float cw = (float)g568.cw, ch = (float)g568.ch, sc = g568.sc, t = (float)frame;
    int i, k;
    if (seed != s568) {
        s568 = seed;
        float mod = (5.0f + 1.4f * gk_hash(seed + 3u)) * sc;
        fg_gear_set(&pin568, 0.0f, 0.0f, mod, 12 + (int)(gk_hash(seed + 4u) * 6.0f), 1, seed, 0);
        fg_gear_set(&top568, 0.0f, 0.0f, mod, 22 + (int)(gk_hash(seed + 5u) * 10.0f), 2, seed, 1);
        for (i = 0; i < NR568; i++) rc568[i][0] = -1.0f;
    }
    float m = pin568.m, P = 3.14159265f * m;
    float tip = m * 0.9f, dep = m * 1.1f;
    /* rack: bar across the frame at ybar (teeth on top), sliding by x0 */
    float ybar = ch * (0.62f + 0.03f * sinf(t * 0.0009f));
    float thick = m * 2.2f;
    float x0 = cw * 0.5f + cw * 0.22f * sinf(t * 0.0038f + gk_hash(seed) * 6.0f);   /* rack offset */
    /* pinion sits on the rack, at a slowly wandering x */
    float px = cw * 0.5f + cw * 0.10f * sinf(t * 0.0013f + 1.0f);
    float py = ybar - pin568.r;
    pin568.cx = px; pin568.cy = py;
    /* rack teeth centres at x0 + k*P; pinion tooth pointing down (+pi/2) aligns with a rack gap */
    float srel = (px - x0) / P - 0.5f;
    pin568.phase = 1.5707963f - (GK_TAU / (float)pin568.n) * (0.25f - srel);
    /* the top gear meshes with the pinion from above-left */
    float tha = -1.5707963f - 0.55f + 0.2f * sinf(t * 0.0011f);
    fg_place(&pin568, &top568, tha);
    fg_gear_colour(&pin568, pal, t, 0.012f);
    fg_gear_colour(&top568, pal, t, 0.012f);
    fg_transfer(&pin568, &top568, tha, 0.08f);
    float amp = gk_smooth((float)sl / 60.0f);
    /* rack colours + transfer with the pinion */
    float hb = fg_pick_sat(pal, gk_hash(seed + 9u) * 32768.0f, 6000.0f);
    float bodyc[3], tg[3];
    fg_colv(pal, hb + 8000.0f, 1.3f, 0.5f, bodyc);
    int k0 = (int)floorf((-x0 - P) / P), k1 = (int)ceilf((cw - x0 + P) / P);
    for (k = k0; k <= k1; k++) {
        int kk = ((k % NR568) + NR568) % NR568;
        fg_colv(pal, hb + (float)kk * 260.0f + 1800.0f * sinf(t * 0.005f + (float)kk * 0.7f), 1.4f, 0.9f, tg);
        if (rc568[kk][0] < 0.0f) { rc568[kk][0] = tg[0]; rc568[kk][1] = tg[1]; rc568[kk][2] = tg[2]; }
        else for (i = 0; i < 3; i++) rc568[kk][i] += (tg[i] - rc568[kk][i]) * 0.01f;
    }
    {   /* engaged: pinion tooth nearest straight down and the rack teeth flanking the gap under it */
        int ip = (int)floorf(fg_q(&pin568, 1.5707963f) + 0.5f);
        ip = ((ip % pin568.n) + pin568.n) % pin568.n;
        int ka = (int)floorf((px - x0) / P), kb = ka + 1;
        int a = ((ka % NR568) + NR568) % NR568, b = ((kb % NR568) + NR568) % NR568;
        for (i = 0; i < 3; i++) {
            float pc = pin568.tc[ip][i], mm = (rc568[a][i] + rc568[b][i]) * 0.5f;
            pin568.tc[ip][i] += (mm - pc) * 0.06f;
            rc568[a][i] += (pc - rc568[a][i]) * 0.06f;
            rc568[b][i] += (pc - rc568[b][i]) * 0.06f;
        }
    }
    /* draw the rack: per pixel over its band */
    {
        int cwi = g568.cw, chi = g568.ch;
        int ya = (int)floorf(ybar - tip - 1.0f), yb = (int)ceilf(ybar + dep + thick + 1.0f);
        if (ya < 0) ya = 0; if (yb >= chi) yb = chi - 1;
        int y, x;
        for (y = ya; y <= yb; y++) {
            float fy = (float)y + 0.5f;
            float *row = g568.acc + ((size_t)y * (size_t)cwi) * 3;
            for (x = 0; x < cwi; x++) {
                float fx = (float)x + 0.5f;
                float q = (fx - x0) / P + 0.25f;
                float fq = floorf(q), u = q - fq;
                int kk = (int)fq; kk = ((kk % NR568) + NR568) % NR568;
                float f = fg_tooth(u);
                float root = ybar + dep;                 /* tooth root line */
                float edge = root - (tip + dep) * f;     /* tooth top */
                float cov, r0, g0, b0;
                if (fy > root + 1.0f) {
                    cov = fg_clamp01(root + thick - fy + 0.5f);
                    float sh = 1.0f - 0.35f * fg_clamp01((fy - root) / thick);
                    r0 = bodyc[0] * sh; g0 = bodyc[1] * sh; b0 = bodyc[2] * sh;
                } else {
                    cov = fg_clamp01(fy - edge + 0.5f);
                    if (cov <= 0.0f) continue;
                    if (f > 0.02f) {
                        float wgt = fg_clamp01((root - fy) / (tip + dep));
                        float mixb = fg_clamp01(1.0f - wgt * 3.0f), br = 0.7f + 0.4f * wgt;
                        r0 = rc568[kk][0] * br * (1.0f - mixb) + bodyc[0] * mixb;
                        g0 = rc568[kk][1] * br * (1.0f - mixb) + bodyc[1] * mixb;
                        b0 = rc568[kk][2] * br * (1.0f - mixb) + bodyc[2] * mixb;
                    } else { r0 = bodyc[0]; g0 = bodyc[1]; b0 = bodyc[2]; }
                }
                cov *= amp;
                if (cov <= 0.0f) continue;
                row[x * 3 + 0] += r0 * cov; row[x * 3 + 1] += g0 * cov; row[x * 3 + 2] += b0 * cov;
            }
        }
    }
    fg_gear_draw(&g568, &pin568, amp);
    fg_gear_draw(&g568, &top568, amp);
    float c[3];
    gk_col(pal, (int)pin568.rb + 2500, 0.4f, amp * 0.6f, c);
    gk_dot(&g568, pin568.cx, pin568.cy, c, 1.5f * sc, 4.0f * sc, 0.3f);
    gk_dot(&g568, top568.cx, top568.cy, c, 1.5f * sc, 4.0f * sc, 0.3f);
    /* a carrier bar linking the two axles */
    gk_seg(&g568, pin568.cx, pin568.cy, top568.cx, top568.cy, c, 1.0f * sc, 3.0f * sc, 0.3f);
    gk_present(&g568, fb, w, h);
}
