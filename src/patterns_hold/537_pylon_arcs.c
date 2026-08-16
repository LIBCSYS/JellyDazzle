/* 537 Pylon Arcs — a line of transmission pylons recedes toward a
 * vanishing point on the right; three sagging wires run pylon to pylon,
 * and along them slow charge pulses travel while short jagged arcs jump
 * between neighbouring wires (each growing over ~15 frames, holding, and
 * fading over ~40, on staggered clocks), lighting the wires around them.
 * Pylons and wires are faint palette silhouettes; arc hue differs per arc
 * and along it; pulses carry the drifting base hue.  Field overlay across
 * the middle band.  Repaint. */
#include "_trace509.h"

#define NPY537 6
#define NAR537 8
#define P537 130

static gk g537;
static gk_bolt b537[NAR537];
static int bi537[NAR537];
static uint32_t bs537 = 0xFFFFFFFFu;
static int as537[NAR537], aw537[NAR537];      /* span index, wire pair (0: 0-1, 1: 1-2) */
static float au537[NAR537], ah537[NAR537];    /* position along span, hue */

/* world: x along the line (0 = nearest pylon .. NPY-1), y up (0 ground), z lateral offset */
static void proj537(float x, float y, float cw, float ch, float *ox, float *oy)
{
    float d = 1.0f + x * 0.6f;
    float p = 1.0f / d;
    *ox = cw * 0.80f - cw * 0.74f * p;
    *oy = ch * 0.5f + ch * 0.44f * p - y * ch * 0.80f * p;
}
/* wire y between pylons at fraction f, sagging */
static inline float sag537(float f) { return 0.72f - 0.05f * 4.0f * f * (1.0f - f); }
static void wirept537(int span, float f, int wire, float cw, float ch, float *ox, float *oy)
{
    float x = (float)span + f;
    float y = sag537(f) + (float)wire * 0.17f - 0.17f;
    proj537(x, y, cw, ch, ox, oy);
}

void pattern_537(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g537, w, h);
    gk_clear(&g537);
    if (seed != bs537) { for (int a = 0; a < NAR537; a++) bi537[a] = -1; bs537 = seed; }
    float cw = (float)g537.cw, ch = (float)g537.ch, sc = g537.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float pc[3], wc[3];
    gk_col(pal, base + 4300, 0.1f, 0.13f, pc);
    gk_col(pal, base + 3600, 0.15f, 0.10f, wc);
    /* pylons */
    for (int k = 0; k < NPY537; k++) {
        float p = 1.0f / (1.0f + (float)k * 0.6f);
        float x0, y0, x1, y1;
        proj537((float)k, 0.0f, cw, ch, &x0, &y0);
        proj537((float)k, 1.0f, cw, ch, &x1, &y1);
        gk_seg(&g537, x0, y0, x1, y1, pc, 1.2f * sc * p, 3.5f * sc * p, 0.4f);
        for (int wv = 0; wv < 3; wv++) {
            float y = 0.72f + (float)wv * 0.17f - 0.17f;
            float ax, ay;
            proj537((float)k, y, cw, ch, &ax, &ay);
            float arm = 0.09f * cw * p * (wv == 1 ? 1.3f : 1.0f);
            gk_seg(&g537, ax - arm, ay, ax + arm, ay, pc, 1.0f * sc * p, 3.0f * sc * p, 0.4f);
        }
    }
    /* wires with travelling pulses */
    for (int wv = 0; wv < 3; wv++) {
        float pulse = (t * 0.0025f + (float)wv * 0.37f);
        pulse -= floorf(pulse);
        pulse *= (float)(NPY537 - 1);
        float pfade = gk_smooth(pulse / 0.4f) * (1.0f - gk_smooth((pulse - (float)(NPY537 - 1) + 0.4f) / 0.4f));
        for (int k = 0; k < NPY537 - 1; k++) {
            float lx = 0.0f, ly = 0.0f;
            for (int i = 0; i <= 12; i++) {
                float f = (float)i / 12.0f, x, y;
                wirept537(k, f, wv, cw, ch, &x, &y);
                if (i > 0) {
                    float pos = (float)k + f, d = pos - pulse;
                    float pl = expf(-d * d * 12.0f) * pfade;
                    float c[3];
                    float pp = 1.0f / (1.0f + pos * 0.6f);
                    gk_col(pal, base + 3600 + wv * 500, 0.15f + 0.4f * pl, 0.10f + 0.5f * pl, c);
                    gk_seg(&g537, lx, ly, x, y, c, (0.7f + 0.8f * pl) * sc * (0.5f + pp), (2.5f + 4.0f * pl) * sc * (0.5f + pp), 0.5f);
                }
                lx = x; ly = y;
            }
        }
        (void)wc;
    }
    /* arcs between wires */
    for (int a = 0; a < NAR537; a++) {
        int P = P537 + a * 13;
        int ph = frame + a * 41;
        int idx = ph / P;
        float age = (float)(ph - idx * P);
        if (bi537[a] != idx) {
            gk_seed(&g537, seed ^ (uint32_t)(idx * 3919 + a * 613));
            as537[a] = (int)(gk_rf(&g537) * (float)(NPY537 - 1)) % (NPY537 - 1);
            aw537[a] = gk_rf(&g537) < 0.5f ? 0 : 1;
            au537[a] = 0.1f + 0.8f * gk_rf(&g537);
            ah537[a] = gk_rf(&g537);
            gk_bolt_gen(&g537, &b537[a], 0.0f, 0.0f, 1.0f, 0.0f, 0.2f, 5, 2, 0.3f);
            bi537[a] = idx;
        }
        float env = gk_env(age, 15.0f, 20.0f, 40.0f);
        if (env <= 0.0f) continue;
        float x0, y0, x1, y1;
        wirept537(as537[a], au537[a], aw537[a], cw, ch, &x0, &y0);
        wirept537(as537[a], au537[a] + 0.03f, aw537[a] + 1, cw, ch, &x1, &y1);
        float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
        float pp = 1.0f / (1.0f + ((float)as537[a] + au537[a]) * 0.6f);
        int pi = base + (int)(ah537[a] * 8000.0f);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.40f * env, h0);
        gk_col(pal, pi + 1500, 0.05f, 0.35f * env, h1);
        gk_col(pal, pi + 300, 0.55f, 0.65f * env, c0);
        gk_col(pal, pi + 1800, 0.45f, 0.6f * env, c1);
        float th = 0.35f + 0.5f * pp;
        bx_draw_grad(&g537, &b537[a], x0, y0, ang, len, 1.0f, age / 15.0f, 1.0f, h0, h1, 0.0f, 1.6f * sc * th, 5.5f * sc * th, 0.5f);
        bx_draw_grad(&g537, &b537[a], x0, y0, ang, len, 1.0f, age / 15.0f, 1.0f, c0, c1, 0.0f, 0.7f * sc * th, 1.9f * sc * th, 0.25f);
        float ec[3];
        gk_col(pal, pi + 600, 0.3f, 0.5f * env, ec);
        gk_dot(&g537, x0, y0, ec, 2.0f * sc * th, 8.0f * sc * th, 0.6f);
        gk_dot(&g537, x1, y1, ec, 2.0f * sc * th, 8.0f * sc * th, 0.6f);
    }
    gk_present(&g537, fb, w, h);
}
