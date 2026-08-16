/* 492 Helix Bolt — two jagged strands wound around each other down the
 * middle of the frame, turning slowly like a twisted cable of lightning.
 * Each strand is a helix sampled with stable per-point jitter, so it can be
 * rebuilt every frame as it turns without any flicker; bright pulses slide
 * down the strands.  Figure overlay in the centre column.  Repaint. */
#include "_hue469.h"

#define NPT492 110

static gk g492;

static void strand492(gk *g, const uint32_t *pal, int base, float phase, float t, uint32_t seed,
                      float cx, float ytop, float ylen, float amp, float sc, int which)
{
    float px[NPT492], py[NPT492];
    for (int i = 0; i < NPT492; i++) {
        float u = (float)i / (float)(NPT492 - 1);
        float ang = phase + u * GK_TAU * 2.2f;
        float jx = (gk_hash((uint32_t)i * 31u + seed + (uint32_t)which * 977u) - 0.5f) * 10.0f * sc;
        float jy = (gk_hash((uint32_t)i * 17u + seed + (uint32_t)which * 313u) - 0.5f) * 6.0f * sc;
        /* depth cue: the strand nearer the viewer is wider apart (perspective) */
        float depth = cosf(ang);
        px[i] = cx + sinf(ang) * amp * (1.0f + 0.15f * depth) + jx;
        py[i] = ytop + u * ylen + jy;
    }
    for (int i = 0; i < NPT492 - 1; i++) {
        float u = (float)i / (float)(NPT492 - 1);
        float depth = cosf(phase + u * GK_TAU * 2.2f);        /* -1 back .. 1 front */
        float front = 0.55f + 0.45f * depth;
        /* pulses travelling down: two per strand */
        float pw = 0.6f + 0.6f * expf(-powf(fmodf(u - t * 0.004f + 3.0f, 0.5f) - 0.25f, 2.0f) * 60.0f);
        float c[3], hc[3];
        int pi = base + (int)(u * 3000.0f) + which * 3500;
        hk_col(pal, pi + 700, 0.05f, 0.45f * front * pw, hc);
        hk_col(pal, pi, 0.40f, 0.7f * front * pw, c);
        gk_seg(g, px[i], py[i], px[i + 1], py[i + 1], hc, 2.0f * sc * front, 6.5f * sc * front, 0.5f);
        gk_seg(g, px[i], py[i], px[i + 1], py[i + 1], c, 0.9f * sc * front, 2.2f * sc * front, 0.25f);
    }
}

void pattern_492(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g492, w, h);
    gk_clear(&g492);
    float cw = (float)g492.cw, ch = (float)g492.ch, sc = g492.sc, t = (float)frame;
    int base = (int)(t * 1.5f) + (int)(seed & 8191u);
    float phase = t * 0.006f;
    float amp = cw * (0.10f + 0.03f * sinf(t * 0.007f));
    float cx = cw * (0.5f + 0.06f * sinf(t * 0.0031f));
    strand492(&g492, pal, base, phase, t, seed, cx, -ch * 0.02f, ch * 1.04f, amp, sc, 0);
    strand492(&g492, pal, base, phase + 3.14159f, t, seed, cx, -ch * 0.02f, ch * 1.04f, amp, sc, 1);
    /* rungs where the strands cross (every half turn) */
    gk_present(&g492, fb, w, h);
}
