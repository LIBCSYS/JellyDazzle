/* _fig541.h — FIGURE kit for patterns 541..571 (objects over black: comets,
 * satellites, kites, cranes ... and the clockwork family of meshed gears).
 * Sits on top of _glow469.h (float canvas, glow segments, tone-map, blit).
 * Everything is static: nothing but pattern_NNN escapes a translation unit.
 *
 * Gears (docs/IDEAS_ARRIVALS.md 4 + 4b):
 *   - a gear = pitch circle r = module*n/2 with n trapezoid teeth; hub hole
 *     and optional spokes so lower layers show through.
 *   - fg_mesh() places gear B's phase from gear A's so the teeth interlock
 *     and B counter-rotates at wA*nA = -wB*nB (internal ring gears co-rotate
 *     at wR*nR = wP*nP).  Mechanics obey physics.
 *   - every tooth owns a colour that chases its own slowly drifting palette
 *     offset, and at each mesh a portion of the engaged teeth's colour is
 *     exchanged, so hue flows through the train.  Colour obeys nothing.
 */
#ifndef JD_FIG541_H
#define JD_FIG541_H
#include "_glow469.h"

#define FG_MAXT 128

typedef struct {
    float cx, cy;        /* centre, canvas px                          */
    float r;             /* pitch radius, canvas px                    */
    float m;             /* module (px per tooth of diameter)          */
    int   n;             /* teeth                                      */
    int   internal;      /* 1 = ring gear, teeth point inward          */
    float hub;           /* hub hole radius (0 = solid)                */
    float rim;           /* spoked: body only outside `rim`; 0 = disc  */
    int   spokes;        /* spoke count when rim > 0                   */
    float outer;         /* internal: outer radius of the ring         */
    float phase;         /* rotation, radians                          */
    float base, step, amp; /* per-tooth palette law (indices)          */
    float bb, rb;        /* body / rim palette bases                   */
    uint32_t hs;         /* colour hash seed                           */
    float tc[FG_MAXT][3];/* tooth colours, chase their targets         */
    float body[3], rimc[3];
    int   cinit;
} fg_gear;

static inline float fg_atan2(float y, float x)
{
    float ax = x < 0.0f ? -x : x, ay = y < 0.0f ? -y : y, a, s, r;
    if (ax < 1e-9f && ay < 1e-9f) return 0.0f;
    a = (ax > ay) ? ay / (ax + 1e-20f) : ax / (ay + 1e-20f);
    s = a * a;
    r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) r = 1.57079637f - r;
    if (x < 0.0f) r = 3.14159274f - r;
    if (y < 0.0f) r = -r;
    return r;
}
static inline float fg_clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }
static inline float fg_fract(float v) { return v - floorf(v); }

/* vivid palette sample: normalised so max channel = 1, then saturation
 * stretched by `sat` (1 = as is), scaled by amp.  Hue is the palette's. */
static inline void fg_colv(const uint32_t *pal, float idx, float sat, float amp, float *out)
{
    uint32_t p = pal[(int)idx & JD_PAL_MASK];
    float r = (float)((p >> 16) & 255), g = (float)((p >> 8) & 255), b = (float)(p & 255);
    float mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    r /= mx; g /= mx; b /= mx;
    float mean = (r + g + b) * (1.0f / 3.0f);
    r = fg_clamp01(mean + (r - mean) * sat);
    g = fg_clamp01(mean + (g - mean) * sat);
    b = fg_clamp01(mean + (b - mean) * sat);
    mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1e-3f) mx = 1e-3f;
    out[0] = r / mx * amp; out[1] = g / mx * amp; out[2] = b / mx * amp;
}
/* index in [start, start+span) whose palette entry is most saturated (16
 * taps).  The palette crossfades every frame, so a per-frame argmax would
 * flip between near-equal candidates and pop the hue (the gate's F1 lesson):
 * the choice is CACHED on (start, span) — callers pass seed-derived, frame-
 * stable inputs and add any time drift to the RESULT, never the input. */
static inline float fg_pick_sat(const uint32_t *pal, float start, float span)
{
    static uint32_t ck[64]; static float cv[64]; static int cn = 0, cinit = 0;
    union { float f; uint32_t u; } a, b;
    a.f = start; b.f = span;
    uint32_t key = a.u * 0x9E3779B1u ^ b.u * 0x85EBCA77u ^ 0x5bd1e995u;
    if (!cinit) { int j; for (j = 0; j < 64; j++) ck[j] = 0xFFFFFFFFu; cinit = 1; }
    int i;
    for (i = 0; i < 64; i++) if (ck[i] == key) return cv[i];
    float best = start, bs = -1.0f;
    for (i = 0; i < 16; i++) {
        float idx = start + span * (float)i / 16.0f;
        uint32_t p = pal[(int)idx & JD_PAL_MASK];
        float r = (float)((p >> 16) & 255), g = (float)((p >> 8) & 255), bl = (float)(p & 255);
        float mx = r > g ? r : g; if (bl > mx) mx = bl;
        float mn = r < g ? r : g; if (bl < mn) mn = bl;
        float sat = mx > 40.0f ? (mx - mn) / mx : 0.0f;
        if (sat > bs) { bs = sat; best = idx; }
    }
    ck[cn & 63] = key; cv[cn & 63] = best; cn++;
    return best;
}

/* trapezoid tooth profile over one pitch u in [0,1): tooth centred at 0.25,
 * gap centred at 0.75; 1 = tip, 0 = root */
static inline float fg_tooth(float u)
{
    float d = u - 0.25f; if (d < 0.0f) d = -d;
    return fg_clamp01((0.30f - d) * (1.0f / 0.14f));
}

/* build a gear.  module in canvas px; kind: 0 solid disc, 1 hub hole,
 * 2 hub + spokes.  Colour law from seed and slot k. */
static void fg_gear_set(fg_gear *g, float cx, float cy, float module, int n, int kind,
                        uint32_t seed, int k)
{
    if (n < 5) n = 5; if (n > FG_MAXT) n = FG_MAXT;
    g->cx = cx; g->cy = cy; g->m = module; g->n = n;
    g->r = module * (float)n * 0.5f;
    g->internal = 0; g->outer = 0.0f;
    g->hub = kind >= 1 ? g->r * 0.16f + module * 0.4f : 0.0f;
    g->rim = kind >= 2 ? g->r * 0.66f : 0.0f;
    g->spokes = 3 + (int)(gk_hash(seed * 31u + (uint32_t)k * 977u + 5u) * 4.0f);
    g->hs = seed * 2654435761u + (uint32_t)k * 40503u;
    g->base = 0.0f;   /* set by fg_gear_colour on first call */
    g->step = (60.0f + 500.0f * gk_hash(g->hs + 2u)) * (gk_hash(g->hs + 9u) < 0.5f ? -1.0f : 1.0f);
    g->amp  = 800.0f + 2400.0f * gk_hash(g->hs + 3u);
    g->cinit = 0;
}
static void fg_ring_set(fg_gear *g, float cx, float cy, float module, int n, float thick,
                        uint32_t seed, int k)
{
    fg_gear_set(g, cx, cy, module, n, 0, seed, k);
    g->internal = 1; g->hub = 0.0f; g->rim = 0.0f;
    g->outer = g->r + module * 1.1f + thick;
}

/* per-frame colour housekeeping: teeth chase drifting palette targets */
static void fg_gear_colour(fg_gear *g, const uint32_t *pal, float t, float rate)
{
    int k;
    float tgt[3];
    if (!g->cinit) {
        g->base = fg_pick_sat(pal, gk_hash(g->hs + 1u) * 32768.0f, 6000.0f);
        g->bb = fg_pick_sat(pal, g->base + 8000.0f, 6000.0f);
        g->rb = fg_pick_sat(pal, g->base + 14000.0f, 6000.0f);
    }
    for (k = 0; k < g->n; k++) {
        float ph = gk_hash(g->hs + 100u + (uint32_t)k) * GK_TAU;
        float rt = 0.004f + 0.010f * gk_hash(g->hs + 300u + (uint32_t)k);
        int idx = (int)(g->base + (float)k * g->step + sinf(t * rt + ph) * g->amp);
        fg_colv(pal, (float)idx, 1.5f, 0.95f, tgt);
        if (!g->cinit) { g->tc[k][0] = tgt[0]; g->tc[k][1] = tgt[1]; g->tc[k][2] = tgt[2]; }
        else {
            g->tc[k][0] += (tgt[0] - g->tc[k][0]) * rate;
            g->tc[k][1] += (tgt[1] - g->tc[k][1]) * rate;
            g->tc[k][2] += (tgt[2] - g->tc[k][2]) * rate;
        }
    }
    fg_colv(pal, g->bb + 700.0f * sinf(t * 0.006f + (float)(g->hs & 7)), 1.4f, 0.5f, g->body);
    fg_colv(pal, g->rb + 700.0f * sinf(t * 0.005f + (float)(g->hs & 5)), 1.4f, 0.75f, g->rimc);
    g->cinit = 1;
}

/* tooth-index position of gear g pointing along world angle th:
 * integer  <=> a tooth centre points along th */
static inline float fg_q(const fg_gear *g, float th)
{
    return (th - g->phase) * (float)g->n / GK_TAU - 0.25f;
}
/* mesh: place b so it interlocks with a; th = direction from a to b.
 * External pair: counter-rotation at n_a/n_b.  Call every frame after
 * a->phase is set (a is the driver or was itself meshed earlier). */
static void fg_mesh(const fg_gear *a, fg_gear *b, float th)
{
    float qa = fg_q(a, th);
    if (b->internal) b->phase = th - (GK_TAU / (float)b->n) * (0.75f + qa);
    else             b->phase = th + 3.14159265f - (GK_TAU / (float)b->n) * (0.75f - qa);
}
/* place b tangent to a along th and mesh (external pair) */
static void fg_place(const fg_gear *a, fg_gear *b, float th)
{
    float d = a->r + b->r;
    b->cx = a->cx + cosf(th) * d; b->cy = a->cy + sinf(th) * d;
    fg_mesh(a, b, th);
}
/* colour transfer at the mesh: the tooth of a nearest th trades colour with
 * the two teeth of b that flank its gap.  k = portion per frame. */
static void fg_transfer(fg_gear *a, fg_gear *b, float th, float k)
{
    int na = a->n, nb = b->n;
    int ia = (int)floorf(fg_q(a, th) + 0.5f);
    float thb = b->internal ? th : th + 3.14159265f;
    float qb = fg_q(b, thb);          /* gap centre => qb = i + 0.5 */
    int ib0 = (int)floorf(qb), ib1 = ib0 + 1;
    ia = ((ia % na) + na) % na; ib0 = ((ib0 % nb) + nb) % nb; ib1 = ((ib1 % nb) + nb) % nb;
    int c;
    for (c = 0; c < 3; c++) {
        float ca = a->tc[ia][c], c0 = b->tc[ib0][c], c1 = b->tc[ib1][c];
        float m = (c0 + c1) * 0.5f;
        a->tc[ia][c] += (m - ca) * k;
        b->tc[ib0][c] += (ca - c0) * k;
        b->tc[ib1][c] += (ca - c1) * k;
    }
}

/* draw a gear additively; amp scales everything (fade in/out) */
static void fg_gear_draw(gk *g, const fg_gear *gr, float amp)
{
    int cw = g->cw, ch = g->ch;
    float tip = gr->m * 0.9f, dep = gr->m * 1.1f;
    float root, R, Rin;
    if (gr->internal) { root = gr->r + dep; R = gr->outer; Rin = gr->r - tip; }
    else              { root = gr->r - dep; R = gr->r + tip; Rin = gr->hub; }
    int xa = (int)floorf(gr->cx - R - 1.0f), xb = (int)ceilf(gr->cx + R + 1.0f);
    int ya = (int)floorf(gr->cy - R - 1.0f), yb = (int)ceilf(gr->cy + R + 1.0f);
    if (xa < 0) xa = 0; if (ya < 0) ya = 0;
    if (xb >= cw) xb = cw - 1; if (yb >= ch) yb = ch - 1;
    if (xa > xb || ya > yb) return;
    float R2 = (R + 1.0f) * (R + 1.0f);
    float Rin2 = Rin > 1.0f ? (Rin - 1.0f) * (Rin - 1.0f) : 0.0f;
    float nT = (float)gr->n / GK_TAU;
    float spk = (float)gr->spokes;
    int x, y;
    for (y = ya; y <= yb; y++) {
        float py = (float)y + 0.5f - gr->cy;
        float *row = g->acc + ((size_t)y * (size_t)cw) * 3;
        for (x = xa; x <= xb; x++) {
            float px = (float)x + 0.5f - gr->cx;
            float d2 = px * px + py * py;
            if (d2 > R2 || d2 < Rin2) continue;
            float d = sqrtf(d2);
            float cov, r0, g0, b0;
            if (!gr->internal) {
                if (d < root - 1.0f) {
                    /* body / spokes */
                    float sh = 0.7f + 0.3f * d / root;
                    if (gr->rim > 0.0f && d < gr->rim - 0.5f) {
                        float a = fg_atan2(py, px);
                        float s = sinf((a - gr->phase) * spk * 0.5f);
                        float wdt = (2.2f + 0.05f * gr->r) / (d + 1.0f);   /* half-width in radians */
                        float sd = (s < 0.0f ? -s : s) - wdt;             /* <0 inside spoke */
                        cov = fg_clamp01(0.5f - sd * d);
                        cov *= fg_clamp01(d - gr->hub + 0.5f);
                        if (cov <= 0.0f) continue;
                        r0 = gr->rimc[0]; g0 = gr->rimc[1]; b0 = gr->rimc[2];
                        cov *= 0.9f;
                    } else {
                        cov = fg_clamp01(d - gr->hub + 0.5f);
                        if (gr->rim > 0.0f) {
                            /* rim band: brighter */
                            float e = fg_clamp01(d - gr->rim + 0.5f);
                            r0 = gr->body[0] * (1.0f - e) + gr->rimc[0] * e;
                            g0 = gr->body[1] * (1.0f - e) + gr->rimc[1] * e;
                            b0 = gr->body[2] * (1.0f - e) + gr->rimc[2] * e;
                        } else { r0 = gr->body[0]; g0 = gr->body[1]; b0 = gr->body[2]; }
                        /* hub ring highlight */
                        if (gr->hub > 0.0f) {
                            float hr = d - gr->hub; if (hr < 3.0f) { float hh = 1.0f + 0.35f * (1.0f - hr / 3.0f); r0 *= hh; g0 *= hh; b0 *= hh; }
                        }
                        r0 *= sh; g0 *= sh; b0 *= sh;
                    }
                } else {
                    float a = fg_atan2(py, px);
                    float q = (a - gr->phase) * nT;
                    float fq = floorf(q);
                    float u = q - fq;
                    int k = (int)fq % gr->n; if (k < 0) k += gr->n;
                    float f = fg_tooth(u);
                    float edge = root + (tip + dep) * f;
                    cov = fg_clamp01(edge - d + 0.5f);
                    if (cov <= 0.0f) continue;
                    if (f > 0.02f) {
                        float wgt = fg_clamp01((d - root) / (tip + dep));  /* 0 root .. 1 tip */
                        float mixb = fg_clamp01(1.0f - wgt * 3.0f);        /* body colour near root */
                        float br = 0.7f + 0.4f * wgt;
                        r0 = (gr->tc[k][0] * br) * (1.0f - mixb) + gr->rimc[0] * mixb;
                        g0 = (gr->tc[k][1] * br) * (1.0f - mixb) + gr->rimc[1] * mixb;
                        b0 = (gr->tc[k][2] * br) * (1.0f - mixb) + gr->rimc[2] * mixb;
                    } else { r0 = gr->rimc[0]; g0 = gr->rimc[1]; b0 = gr->rimc[2]; }
                }
            } else {
                /* ring gear: teeth inward from root, body outward to R */
                if (d > root + 1.0f) {
                    cov = fg_clamp01(R - d + 0.5f);
                    float e = fg_clamp01((d - root) / (R - root + 1e-3f));
                    float sh = 1.0f - 0.35f * e;
                    r0 = gr->rimc[0] * sh; g0 = gr->rimc[1] * sh; b0 = gr->rimc[2] * sh;
                } else {
                    float a = fg_atan2(py, px);
                    float q = (a - gr->phase) * nT;
                    float fq = floorf(q);
                    float u = q - fq;
                    int k = (int)fq % gr->n; if (k < 0) k += gr->n;
                    float f = fg_tooth(u);
                    float edge = root - (tip + dep) * f;
                    cov = fg_clamp01(d - edge + 0.5f);
                    if (cov <= 0.0f) continue;
                    if (f > 0.02f) {
                        float wgt = fg_clamp01((root - d) / (tip + dep));
                        float mixb = fg_clamp01(1.0f - wgt * 3.0f);
                        float br = 0.7f + 0.4f * wgt;
                        r0 = (gr->tc[k][0] * br) * (1.0f - mixb) + gr->rimc[0] * mixb;
                        g0 = (gr->tc[k][1] * br) * (1.0f - mixb) + gr->rimc[1] * mixb;
                        b0 = (gr->tc[k][2] * br) * (1.0f - mixb) + gr->rimc[2] * mixb;
                    } else { r0 = gr->rimc[0]; g0 = gr->rimc[1]; b0 = gr->rimc[2]; }
                }
            }
            cov *= amp;
            row[x * 3 + 0] += r0 * cov;
            row[x * 3 + 1] += g0 * cov;
            row[x * 3 + 2] += b0 * cov;
        }
    }
}

/* ---- filled convex polygon, soft edges, additive ---------------------- */
static void fg_poly_fill(gk *g, const float *xs, const float *ys, int n, const float *c)
{
    int cw = g->cw, ch = g->ch, i, x, y;
    if (n < 3) return;
    float mnx = xs[0], mxx = xs[0], mny = ys[0], mxy = ys[0];
    for (i = 1; i < n; i++) {
        if (xs[i] < mnx) mnx = xs[i]; if (xs[i] > mxx) mxx = xs[i];
        if (ys[i] < mny) mny = ys[i]; if (ys[i] > mxy) mxy = ys[i];
    }
    int xa = (int)floorf(mnx) - 1, xb = (int)ceilf(mxx) + 1;
    int ya = (int)floorf(mny) - 1, yb = (int)ceilf(mxy) + 1;
    if (xa < 0) xa = 0; if (ya < 0) ya = 0;
    if (xb >= cw) xb = cw - 1; if (yb >= ch) yb = ch - 1;
    if (xa > xb || ya > yb) return;
    /* winding: make edges point so inside is on the left (positive) */
    float area = 0.0f;
    for (i = 0; i < n; i++) { int j = (i + 1) % n; area += xs[i] * ys[j] - xs[j] * ys[i]; }
    float sgn = area >= 0.0f ? 1.0f : -1.0f;
    float ex[16], ey[16], ec[16];
    if (n > 16) n = 16;
    for (i = 0; i < n; i++) {
        int j = (i + 1) % n;
        float dx = xs[j] - xs[i], dy = ys[j] - ys[i];
        float l = sqrtf(dx * dx + dy * dy); if (l < 1e-6f) l = 1e-6f;
        /* inward normal */
        ex[i] = -dy / l * sgn; ey[i] = dx / l * sgn;
        ec[i] = -(ex[i] * xs[i] + ey[i] * ys[i]);
    }
    for (y = ya; y <= yb; y++) {
        float fy = (float)y + 0.5f;
        float *row = g->acc + ((size_t)y * (size_t)cw) * 3;
        for (x = xa; x <= xb; x++) {
            float fx = (float)x + 0.5f, cov = 1.0f;
            for (i = 0; i < n; i++) {
                float d = ex[i] * fx + ey[i] * fy + ec[i] + 0.5f;
                if (d < cov) cov = d;
                if (cov <= 0.0f) break;
            }
            if (cov <= 0.0f) continue;
            row[x * 3 + 0] += c[0] * cov;
            row[x * 3 + 1] += c[1] * cov;
            row[x * 3 + 2] += c[2] * cov;
        }
    }
}
static void fg_tri(gk *g, float x0, float y0, float x1, float y1, float x2, float y2, const float *c)
{
    float xs[3] = { x0, x1, x2 }, ys[3] = { y0, y1, y2 };
    fg_poly_fill(g, xs, ys, 3, c);
}
/* soft filled ellipse (rotated), additive */
static void fg_ellipse(gk *g, float cx, float cy, float ax, float ay, float rot, const float *c)
{
    int cw = g->cw, ch = g->ch;
    float R = ax > ay ? ax : ay;
    int xa = (int)floorf(cx - R) - 1, xb = (int)ceilf(cx + R) + 1;
    int ya = (int)floorf(cy - R) - 1, yb = (int)ceilf(cy + R) + 1;
    if (xa < 0) xa = 0; if (ya < 0) ya = 0;
    if (xb >= cw) xb = cw - 1; if (yb >= ch) yb = ch - 1;
    float cr = cosf(rot), sr = sinf(rot);
    float iax = 1.0f / (ax > 0.5f ? ax : 0.5f), iay = 1.0f / (ay > 0.5f ? ay : 0.5f);
    float mn = ax < ay ? ax : ay; if (mn < 0.5f) mn = 0.5f;
    int x, y;
    for (y = ya; y <= yb; y++) {
        float py = (float)y + 0.5f - cy;
        float *row = g->acc + ((size_t)y * (size_t)cw) * 3;
        for (x = xa; x <= xb; x++) {
            float px = (float)x + 0.5f - cx;
            float u = (px * cr + py * sr) * iax, v = (-px * sr + py * cr) * iay;
            float q = sqrtf(u * u + v * v);
            float cov = fg_clamp01((1.0f - q) * mn + 0.5f);
            if (cov <= 0.0f) continue;
            row[x * 3 + 0] += c[0] * cov;
            row[x * 3 + 1] += c[1] * cov;
            row[x * 3 + 2] += c[2] * cov;
        }
    }
}
/* polyline glow through n points */
static void fg_polyline(gk *g, const float *xs, const float *ys, int n, const float *c,
                        float cr, float gr, float gi)
{
    int i;
    for (i = 0; i + 1 < n; i++) gk_seg(g, xs[i], ys[i], xs[i + 1], ys[i + 1], c, cr, gr, gi);
}
/* rotated ellipse outline (for rings seen at an angle) */
static void fg_ellipse_ring(gk *g, float cx, float cy, float ax, float ay, float rot,
                            const float *c, float cr, float gr, float gi, int segs)
{
    float cr_ = cosf(rot), sr = sinf(rot), lx = 0.0f, ly = 0.0f;
    int i;
    for (i = 0; i <= segs; i++) {
        float a = GK_TAU * (float)i / (float)segs;
        float u = cosf(a) * ax, v = sinf(a) * ay;
        float x = cx + u * cr_ - v * sr, y = cy + u * sr + v * cr_;
        if (i) gk_seg(g, lx, ly, x, y, c, cr, gr, gi);
        lx = x; ly = y;
    }
}
/* rotate a point (x,y) about origin by (ca, sa) and translate */
static inline void fg_xf(float x, float y, float ca, float sa, float ox, float oy, float *px, float *py)
{
    *px = ox + x * ca - y * sa; *py = oy + x * sa + y * ca;
}
/* palette colour by float index, whitened wt, amp */
static inline void fg_col(const uint32_t *pal, float idx, float wt, float amp, float *out)
{
    gk_col(pal, (int)idx, wt, amp, out);
}
/* smooth periodic fade for an object with life period P frames: 0 at both
 * ends, 1 in the middle, eased over `edge` frames */
static inline float fg_life(float age, float P, float edge)
{
    if (age <= 0.0f || age >= P) return 0.0f;
    float a = age < edge ? age / edge : 1.0f;
    float b = P - age < edge ? (P - age) / edge : 1.0f;
    return gk_smooth(a) * gk_smooth(b);
}
#endif
