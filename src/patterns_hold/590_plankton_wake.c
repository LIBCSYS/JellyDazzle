/* 590 Plankton Wake — a field of dormant motes over black; three unseen
 * swimmers glide slow curves through it and every mote they pass lights up
 * and fades over a few seconds, so glowing wakes trail the invisible.  Hues
 * from a narrow palette window per segment; brighter motes shift warmer.
 * Repaint pattern (excitation state is per mote). */
#include "_spark572.h"

#define NP590 620

static gk g590;
static float px590[NP590], py590[NP590], pe590[NP590], ph590[NP590], ps590[NP590];
static uint32_t bs590 = 0xFFFFFFFFu;
static int base590;
static float hue0590;

void pattern_590(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, s;
    gk_setup(&g590, w, h);
    gk_clear(&g590);
    float cw = (float)g590.cw, ch = (float)g590.ch, sc = g590.sc;
    if (seed != bs590) {
        base590 = (int)(seed & 0x7FFFu);
        hue0590 = gk_hash(seed ^ 0x590u);
        for (i = 0; i < NP590; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 6007);
            px590[i] = gk_hash(r + 1u) * cw; py590[i] = gk_hash(r + 2u) * ch;
            pe590[i] = 0.0f; ph590[i] = gk_hash(r + 3u);
            ps590[i] = (0.7f + 1.1f * gk_hash(r + 4u)) * sc;
        }
        bs590 = seed;
    }
    float t = (float)frame;
    float R = ch * 0.11f, iR2 = 1.0f / (R * R);
    float swx[3], swy[3];
    for (s = 0; s < 3; s++) {
        swx[s] = cw * (0.5f + 1.5f * (gk_noise1(t * 0.0065f + (float)s * 31.0f, 100u + (uint32_t)s) - 0.5f));
        swy[s] = ch * (0.5f + 1.5f * (gk_noise1(t * 0.0065f + (float)s * 17.0f, 200u + (uint32_t)s) - 0.5f));
    }
    float col[3];
    for (i = 0; i < NP590; i++) {
        float e = pe590[i] * 0.992f;
        for (s = 0; s < 3; s++) {
            float dx = px590[i] - swx[s], dy = py590[i] - swy[s];
            float q = 1.0f - (dx * dx + dy * dy) * iR2;
            if (q > 0.0f) e += (1.0f - e) * q * 0.3f;
        }
        pe590[i] = e;
        /* slow ambient shimmer so the field is faintly present */
        float amb = 0.09f + 0.06f * gk_noise1(t * 0.01f + ph590[i] * 50.0f, (uint32_t)i);
        float b = amb + e * 1.7f;
        sk_col(pal, sk_hidx(base590, hue0590 + ph590[i] * 0.10f + e * 0.06f + t * 0.00003f), 0.3f * e, 0.55f, b, col);
        gk_dot(&g590, px590[i], py590[i], col, ps590[i] * (1.0f + 0.4f * e), ps590[i] * (2.5f + 2.5f * e), 0.35f);
    }
    gk_present(&g590, fb, w, h);
}
