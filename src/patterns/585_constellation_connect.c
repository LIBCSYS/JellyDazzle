/* 585 Constellation Connect — a fixed sky of slow-twinkling stars; every so
 * often a chain of neighbouring stars is joined by lines that grow star to
 * star over a couple of seconds, hold, and fade, while another chain begins
 * elsewhere.  Stars carry palette hues; each constellation's lines take the
 * hue of the star they leave.  Repaint pattern. */
#include "_spark572.h"

#define NS585 72
#define NC585 3
#define CL585 7

static gk g585;
static float sx585[NS585], sy585[NS585], sh585[NS585], sph585[NS585], ssz585[NS585];
static int chain585[NC585][CL585], clen585[NC585], cstart585[NC585];
static uint32_t bs585 = 0xFFFFFFFFu;
static int base585;

static void build585(int c, uint32_t r)
{
    int used[NS585] = {0}, n = 0, cur = (int)(gk_hash(r + 1u) * (NS585 - 0.01f));
    chain585[c][n++] = cur; used[cur] = 1;
    while (n < CL585) {
        int best = -1, k; float bd = 1e30f;
        for (k = 0; k < NS585; k++) {
            if (used[k]) continue;
            float dx = sx585[k] - sx585[cur], dy = sy585[k] - sy585[cur];
            float d = dx * dx + dy * dy + gk_hash(r + (uint32_t)(k * 31 + n)) * 900.0f;
            if (d < bd) { bd = d; best = k; }
        }
        if (best < 0) break;
        chain585[c][n++] = best; used[best] = 1; cur = best;
    }
    clen585[c] = 4 + (int)(gk_hash(r + 9u) * (float)(CL585 - 3));
    if (clen585[c] > n) clen585[c] = n;
}

void pattern_585(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, c;
    gk_setup(&g585, w, h);
    gk_clear(&g585);
    float cw = (float)g585.cw, ch = (float)g585.ch, sc = g585.sc;
    if (seed != bs585) {
        base585 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NS585; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 7877);
            sx585[i] = cw * (0.03f + 0.94f * gk_hash(r + 1u));
            sy585[i] = ch * (0.03f + 0.94f * gk_hash(r + 2u));
            sh585[i] = gk_hash(r + 3u);
            sph585[i] = gk_hash(r + 4u) * 100.0f;
            ssz585[i] = (1.0f + 1.6f * gk_hash(r + 5u)) * sc;
        }
        for (c = 0; c < NC585; c++) { build585(c, seed ^ (uint32_t)(c * 977)); cstart585[c] = frame - c * 220; }
        bs585 = seed;
    }
    float t = (float)frame;
    float col[3];
    for (i = 0; i < NS585; i++) {
        float tw = 0.55f + 0.45f * gk_noise1(t * 0.012f + sph585[i], (uint32_t)i + 7u);
        sk_col(pal, sk_hidx(base585, sh585[i] + t * 0.00003f), 0.3f, 0.5f, tw * 1.2f, col);
        gk_dot(&g585, sx585[i], sy585[i], col, ssz585[i], ssz585[i] * 3.5f, 0.35f);
    }
    for (c = 0; c < NC585; c++) {
        float age = t - (float)cstart585[c];
        float grow = 70.0f * (float)clen585[c] / 5.0f;
        float total = grow + 260.0f + 120.0f;
        if (age > total) { build585(c, seed ^ (uint32_t)(frame * 131 + c * 977)); cstart585[c] = frame; age = 0.0f; }
        float env = age > grow + 260.0f ? 1.0f - (age - grow - 260.0f) / 120.0f : 1.0f;
        env = env * env * (3.0f - 2.0f * env);
        float prog = age / grow * (float)(clen585[c] - 1);
        int k;
        for (k = 0; k < clen585[c] - 1; k++) {
            float f = prog - (float)k;
            if (f <= 0.0f) break;
            if (f > 1.0f) f = 1.0f;
            f = f * f * (3.0f - 2.0f * f);
            int a = chain585[c][k], b = chain585[c][k + 1];
            float x1 = sx585[a] + (sx585[b] - sx585[a]) * f, y1 = sy585[a] + (sy585[b] - sy585[a]) * f;
            sk_col(pal, sk_hidx(base585, sh585[a] + t * 0.00003f), 0.15f, 0.5f, 0.45f * env, col);
            sk_line(&g585, sx585[a], sy585[a], x1, y1, 1.3f * sc, col);
            /* the star just reached brightens */
            if (f >= 1.0f) {
                sk_col(pal, sk_hidx(base585, sh585[b]), 0.4f, 0.5f, 0.6f * env, col);
                gk_dot(&g585, sx585[b], sy585[b], col, ssz585[b] * 1.2f, ssz585[b] * 5.0f, 0.5f);
            }
        }
    }
    gk_present(&g585, fb, w, h);
}
