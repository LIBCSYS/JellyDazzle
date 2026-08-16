/* 566 Sprocket Chain — two sprockets of different size joined by a roller
 * chain: the chain runs round both at one speed, so the sprockets turn the
 * same way at speeds inverse to their radii, and every link seats in a
 * tooth gap exactly (chain pitch = tooth pitch).  Every link and every
 * tooth owns a drifting hue; links trade colour with the teeth they seat
 * in and carry it across to the other sprocket.  Figure overlay, repaint. */
#include "_fig541.h"

#define NL566 160
static gk g566;
static fg_gear A566, B566;
static float lc566[NL566][3];
static uint32_t s566 = 0xFFFFFFFFu;
static float s0_566;

void pattern_566(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g566, w, h);
    gk_clear(&g566);
    float cw = (float)g566.cw, ch = (float)g566.ch, sc = g566.sc, t = (float)frame;
    int i, k;
    if (seed != s566) {
        s566 = seed;
        float mod = (4.0f + 1.0f * gk_hash(seed + 3u)) * sc;
        int na = 26 + (int)(gk_hash(seed + 4u) * 12.0f), nb = 10 + (int)(gk_hash(seed + 5u) * 6.0f);
        fg_gear_set(&A566, 0.0f, 0.0f, mod, na, 2, seed, 0);
        fg_gear_set(&B566, 0.0f, 0.0f, mod, nb, 1, seed, 1);
        s0_566 = 0.0f;
        for (i = 0; i < NL566; i++) lc566[i][0] = lc566[i][1] = lc566[i][2] = -1.0f;
    }
    float P = 3.14159265f * A566.m;                  /* chain pitch = tooth pitch */
    /* sprocket centres: A left, B right, gently swaying */
    float tiltc = 0.18f * sinf(t * 0.0009f);
    float span = cw * 0.42f;
    float mx = cw * 0.5f + cw * 0.02f * sinf(t * 0.0013f), my = ch * 0.5f + ch * 0.03f * sinf(t * 0.0007f);
    A566.cx = mx - cosf(tiltc) * span * 0.5f; A566.cy = my - sinf(tiltc) * span * 0.5f;
    B566.cx = mx + cosf(tiltc) * span * 0.5f; B566.cy = my + sinf(tiltc) * span * 0.5f;
    float rA = A566.r, rB = B566.r;
    float dx = B566.cx - A566.cx, dy = B566.cy - A566.cy, d = sqrtf(dx * dx + dy * dy);
    float ux = dx / d, uy = dy / d, nx = -uy, ny = ux;
    float cph, sph, n1x, n1y, n2x, n2y, thu, phi, a1, a2, Lt, arcB, arcA, L;
    /* the chain must close on a whole number of links: nudge the centre
     * distance until the loop length is a multiple of the pitch */
    int it;
    float Ltarget = 0.0f;
    for (it = 0; it < 4; it++) {
        cph = (rA - rB) / d; sph = sqrtf(1.0f - cph * cph);
        phi = atan2f(sph, cph);
        Lt = d * sph; arcB = 2.0f * phi * rB; arcA = (GK_TAU - 2.0f * phi) * rA;
        L = 2.0f * Lt + arcA + arcB;
        if (it == 0) Ltarget = floorf(L / P + 0.5f) * P;
        d += (Ltarget - L) * 0.5f;
    }
    B566.cx = A566.cx + ux * d; B566.cy = A566.cy + uy * d;
    n1x = ux * cph + nx * sph; n1y = uy * cph + ny * sph;    /* tangent normal 1 */
    n2x = ux * cph - nx * sph; n2y = uy * cph - ny * sph;    /* tangent normal 2 */
    thu = atan2f(uy, ux);
    a1 = thu + phi; a2 = thu - phi;                          /* angles of n1, n2 */
    /* tangent lines: A+rA*n -> B+rB*n; their direction is not u when rA != rB */
    float t1x = (B566.cx + rB * n1x - A566.cx - rA * n1x) / Lt, t1y = (B566.cy + rB * n1y - A566.cy - rA * n1y) / Lt;
    float t2x = (A566.cx + rA * n2x - B566.cx - rB * n2x) / Lt, t2y = (A566.cy + rA * n2y - B566.cy - rB * n2y) / Lt;
    /* chain advance: breathes, never stops */
    float v = (0.55f + 0.25f * sinf(t * 0.0021f)) * sc;
    s0_566 += v;
    if (sl < 2) s0_566 = 0.0f;
    if (s0_566 > L) s0_566 -= L;
    /* sprocket phases follow the chain (same rotation sense, w = v/r) */
    float sB0 = Lt, sA0 = Lt + arcB + Lt;                          /* path offsets where the arcs start */
    B566.phase = a1 - (s0_566 - sB0) / rB - 0.75f * GK_TAU / (float)B566.n;
    A566.phase = a2 - (s0_566 - sA0) / rA - 0.75f * GK_TAU / (float)A566.n;
    fg_gear_colour(&A566, pal, t, 0.012f);
    fg_gear_colour(&B566, pal, t, 0.012f);
    float amp = gk_smooth((float)sl / 60.0f);
    fg_gear_draw(&g566, &A566, amp);
    fg_gear_draw(&g566, &B566, amp);
    /* links */
    int nl = (int)floorf(L / P + 0.5f); if (nl > NL566) nl = NL566;
    float hb = fg_pick_sat(pal, gk_hash(seed + 9u) * 32768.0f, 6000.0f);
    float px = 0.0f, py = 0.0f, c[3];
    for (k = 0; k <= nl; k++) {
        int kk = k % nl;
        float s = s0_566 + (float)kk * P;
        s -= L * floorf(s / L);
        float x, y; int seat = -1; float ang = 0.0f;
        if (s < Lt) { x = A566.cx + rA * n1x + t1x * s; y = A566.cy + rA * n1y + t1y * s; }
        else if (s < Lt + arcB) { ang = a1 - (s - Lt) / rB; x = B566.cx + cosf(ang) * rB; y = B566.cy + sinf(ang) * rB; seat = 1; }
        else if (s < Lt + arcB + Lt) { float q = s - Lt - arcB; x = B566.cx + rB * n2x + t2x * q; y = B566.cy + rB * n2y + t2y * q; }
        else { ang = a2 - (s - sA0) / rA; x = A566.cx + cosf(ang) * rA; y = A566.cy + sinf(ang) * rA; seat = 0; }
        /* link colour: chase a drifting palette target */
        float tg[3];
        fg_colv(pal, hb + (float)kk * 180.0f + 2000.0f * sinf(t * 0.006f + (float)kk * 0.4f), 1.4f, 0.9f, tg);
        if (lc566[kk][0] < 0.0f) { lc566[kk][0] = tg[0]; lc566[kk][1] = tg[1]; lc566[kk][2] = tg[2]; }
        else for (i = 0; i < 3; i++) lc566[kk][i] += (tg[i] - lc566[kk][i]) * 0.01f;
        /* seated in a gap: trade colour with the two flanking teeth */
        if (seat >= 0 && k < nl) {
            fg_gear *g = seat ? &B566 : &A566;
            float q = fg_q(g, ang);
            int i0 = (int)floorf(q), i1 = i0 + 1;
            i0 = ((i0 % g->n) + g->n) % g->n; i1 = ((i1 % g->n) + g->n) % g->n;
            for (i = 0; i < 3; i++) {
                float m = (g->tc[i0][i] + g->tc[i1][i]) * 0.5f, l = lc566[kk][i];
                lc566[kk][i] += (m - l) * 0.06f;
                g->tc[i0][i] += (l - g->tc[i0][i]) * 0.06f;
                g->tc[i1][i] += (l - g->tc[i1][i]) * 0.06f;
            }
        }
        c[0] = lc566[kk][0] * amp; c[1] = lc566[kk][1] * amp; c[2] = lc566[kk][2] * amp;
        if (k) gk_seg(&g566, px, py, x, y, c, 1.1f * sc, 2.6f * sc, 0.3f);      /* side plate */
        gk_dot(&g566, x, y, c, 1.7f * sc, 3.5f * sc, 0.35f);                    /* roller */
        px = x; py = y;
    }
    gk_col(pal, (int)A566.rb + 2500, 0.4f, amp * 0.6f, c);
    gk_dot(&g566, A566.cx, A566.cy, c, 1.6f * sc, 4.0f * sc, 0.3f);
    gk_dot(&g566, B566.cx, B566.cy, c, 1.4f * sc, 3.5f * sc, 0.3f);
    gk_present(&g566, fb, w, h);
}
