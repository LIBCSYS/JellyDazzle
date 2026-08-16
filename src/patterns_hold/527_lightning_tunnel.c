/* 527 Lightning Tunnel — a tunnel of jagged rings receding to a vanishing
 * point, seen from inside; the rings drift very slowly toward the viewer
 * (fading in far away, fading out as they pass), each ring a closed
 * discharge with stable jag, a bright charge crawling round it, and four
 * jagged spars linking it to the next ring, the whole tube turning
 * gently.  Each ring has its own hue; hue also runs round each ring with
 * the charge; everything drifts with time.  Figure overlay, centre-
 * weighted, black between the rings.  Repaint. */
#include "_trace509.h"

#define NR527 9
#define NP527 64

static gk g527;

void pattern_527(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g527, w, h);
    gk_clear(&g527);
    float cw = (float)g527.cw, ch = (float)g527.ch, sc = g527.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float cx = cw * 0.5f + cw * 0.05f * sinf(t * 0.0023f), cy = ch * 0.5f + ch * 0.05f * cosf(t * 0.0019f);
    float R0 = (cw < ch ? cw : ch) * 0.34f;
    float zoom = t * 0.0035f;                    /* rings per frame toward the viewer */
    float rot = t * 0.0012f;
    float pxs[NP527], pys[NP527], lxs[NP527], lys[NP527];
    int have_last = 0;
    /* far ring first so near ones lie on top */
    for (int k = NR527 - 1; k >= 0; k--) {
        float zf = (float)k + 1.0f - (zoom - floorf(zoom));       /* 0.. NR527 */
        int rid = (int)floorf(zoom) + k;                          /* ring identity, stable as it moves */
        float z = 0.25f + zf * 0.75f;
        float rad = R0 / z * 1.4f;
        float near = gk_smooth((zf - 0.25f) / 0.6f);              /* fade out as it passes */
        float far = 1.0f - gk_smooth((zf - (float)NR527 + 2.5f) / 2.0f);
        float amp = near * far * (0.35f + 0.65f / z);
        if (amp <= 0.002f) { have_last = 0; continue; }
        uint32_t hs = (uint32_t)rid * 2857u + seed;
        int pi = base + (int)(gk_hash(hs + 1u) * 8000.0f);
        float charge = gk_hash(hs + 2u) * GK_TAU + t * 0.02f * (rid & 1 ? 1.0f : -1.0f);
        float rr = rot * (1.0f + 0.2f * gk_hash(hs + 3u));
        for (int i = 0; i < NP527; i++) {
            float a = GK_TAU * ((float)i + (gk_hash((uint32_t)i * 59u + hs) - 0.5f) * 0.7f) / (float)NP527 + rr;
            float j = (gk_hash((uint32_t)i * 47u + hs) - 0.5f) * 0.14f * rad + (gk_noise1(t * 0.02f + (float)i * 2.3f, hs) - 0.5f) * 0.04f * rad;
            pxs[i] = cx + cosf(a) * (rad + j); pys[i] = cy + sinf(a) * (rad + j);
        }
        float th = 0.6f + 1.0f / z;
        for (int i = 0; i < NP527; i++) {
            int i2 = (i + 1) % NP527;
            float a = GK_TAU * (float)i / (float)NP527 + rr;
            float da = a - charge; da -= GK_TAU * floorf(da / GK_TAU + 0.5f);
            float ch2 = expf(-da * da * 2.5f);
            int pj = pi + (int)((float)i / (float)NP527 * 3000.0f);
            float c[3], hc[3];
            gk_col(pal, pj + 700, 0.05f, 0.30f * amp * (0.6f + 0.8f * ch2), hc);
            gk_col(pal, pj, 0.35f + 0.4f * ch2, 0.5f * amp * (0.5f + 1.0f * ch2), c);
            gk_seg(&g527, pxs[i], pys[i], pxs[i2], pys[i2], hc, 2.0f * sc * th, 6.5f * sc * th, 0.5f);
            gk_seg(&g527, pxs[i], pys[i], pxs[i2], pys[i2], c, 0.9f * sc * th, 2.2f * sc * th, 0.25f);
        }
        /* spars to the previous (farther) ring */
        if (have_last) {
            for (int s = 0; s < 4; s++) {
                int i = (s * NP527) / 4 + (int)(gk_hash(hs + 9u) * 5.0f);
                i %= NP527;
                float sc0[3], sh0[3];
                gk_col(pal, pi + 1500, 0.3f, 0.35f * amp, sc0);
                gk_col(pal, pi + 2200, 0.05f, 0.20f * amp, sh0);
                float mx = 0.5f * (pxs[i] + lxs[i]) + (gk_hash(hs + 20u + (uint32_t)s) - 0.5f) * 0.15f * rad;
                float my = 0.5f * (pys[i] + lys[i]) + (gk_hash(hs + 30u + (uint32_t)s) - 0.5f) * 0.15f * rad;
                gk_seg(&g527, pxs[i], pys[i], mx, my, sh0, 1.4f * sc * th, 4.5f * sc * th, 0.5f);
                gk_seg(&g527, mx, my, lxs[i], lys[i], sh0, 1.4f * sc * th, 4.5f * sc * th, 0.5f);
                gk_seg(&g527, pxs[i], pys[i], mx, my, sc0, 0.6f * sc * th, 1.5f * sc * th, 0.25f);
                gk_seg(&g527, mx, my, lxs[i], lys[i], sc0, 0.6f * sc * th, 1.5f * sc * th, 0.25f);
            }
        }
        memcpy(lxs, pxs, sizeof lxs); memcpy(lys, pys, sizeof lys);
        have_last = 1;
    }
    float vc[3];
    gk_col(pal, base + 3000, 0.3f, 0.5f, vc);
    gk_dot(&g527, cx, cy, vc, 3.0f * sc, 14.0f * sc, 0.6f);
    gk_present(&g527, fb, w, h);
}
