/* 596 Frost Glints — a jittered field of ice points across the whole frame,
 * dark until slow patches of light drift over them: where a patch passes,
 * each point blooms into a small four-armed glint and dims again.  Hues
 * follow the patch field, so each drift carries its own colour.  Repaint
 * pattern. */
#include "_spark572.h"

#define NG596 520

static gk g596;
static float gx596[NG596], gy596[NG596], gp596[NG596], gr596[NG596];
static uint32_t bs596 = 0xFFFFFFFFu;
static int base596;

void pattern_596(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g596, w, h);
    gk_clear(&g596);
    float cw = (float)g596.cw, ch = (float)g596.ch, sc = g596.sc;
    if (seed != bs596) {
        base596 = (int)(seed & 0x7FFFu);
        int cols = 29, rows = NG596 / cols + 1;
        for (i = 0; i < NG596; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 3331);
            gx596[i] = ((float)(i % cols) + 0.1f + 0.8f * gk_hash(r + 1u)) * cw / (float)cols;
            gy596[i] = ((float)(i / cols) + 0.1f + 0.8f * gk_hash(r + 2u)) * ch / (float)rows;
            gp596[i] = gk_hash(r + 3u);
            gr596[i] = (0.7f + 0.6f * gk_hash(r + 4u)) * sc;
        }
        bs596 = seed;
    }
    float t = (float)frame;
    float hue0 = gk_hash(seed ^ 0x596u) + t * 0.00003f;
    float col[3], c2[3];
    for (i = 0; i < NG596; i++) {
        float x = gx596[i], y = gy596[i];
        float n = gk_noise2(x * 0.007f / sc + t * 0.0025f, y * 0.007f / sc + t * 0.0012f, 61u);
        n = 0.6f * n + 0.4f * gk_noise2(x * 0.02f / sc - t * 0.002f, y * 0.02f / sc + gp596[i], 62u);
        float g = (n - 0.48f) / 0.26f;
        if (g <= 0.0f) { g = 0.0f; }
        if (g > 1.0f) g = 1.0f;
        g = g * g * (3.0f - 2.0f * g);
        float b = 0.06f + 0.94f * g;
        float hue = hue0 + n * 0.5f + gp596[i] * 0.04f;
        sk_col(pal, sk_hidx(base596, hue), 0.4f * g, 0.5f, b, col);
        gk_dot(&g596, x, y, col, gr596[i], gr596[i] * (2.0f + 3.0f * g), 0.35f);
        if (g > 0.05f) {
            float L = (1.5f + 5.5f * g) * gr596[i];
            float m = 0.55f * g;
            c2[0] = col[0] * m; c2[1] = col[1] * m; c2[2] = col[2] * m;
            sk_line(&g596, x - L, y, x + L, y, 1.0f, c2);
            sk_line(&g596, x, y - L, x, y + L, 1.0f, c2);
        }
    }
    gk_present(&g596, fb, w, h);
}
