/* 592 Twinkle Net — a loose triangular net of faint threads whose knots
 * twinkle in slow waves that roll across the frame; the net itself sways
 * as if hung in a draught.  Knots take hues along a palette drift keyed to
 * position, so colour crosses the net like the wave.  Repaint pattern. */
#include "_spark572.h"

#define GW592 16
#define GH592 12

static gk g592;
static float nx592[GH592][GW592], ny592[GH592][GW592];
static uint32_t bs592 = 0xFFFFFFFFu;
static int base592;

void pattern_592(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, j;
    gk_setup(&g592, w, h);
    gk_clear(&g592);
    float cw = (float)g592.cw, ch = (float)g592.ch, sc = g592.sc;
    if (seed != bs592) { base592 = (int)(seed & 0x7FFFu); bs592 = seed; }
    float t = (float)frame;
    float dx = cw / (GW592 - 1.6f), dy = ch / (GH592 - 1.6f);
    for (j = 0; j < GH592; j++)
        for (i = 0; i < GW592; i++) {
            float bx = ((float)i + (j & 1 ? 0.5f : 0.0f) - 0.3f) * dx, by = ((float)j - 0.3f) * dy;
            nx592[j][i] = bx + (gk_noise2((float)i * 0.4f, (float)j * 0.4f + t * 0.004f, 3u) - 0.5f) * dx * 0.55f;
            ny592[j][i] = by + (gk_noise2((float)i * 0.4f + 9.0f, (float)j * 0.4f + t * 0.004f, 4u) - 0.5f) * dy * 0.55f;
        }
    float wave_a = gk_noise1(t * 0.0015f, 8u) * GK_TAU;
    float kx = cosf(wave_a) / cw * 6.0f, ky = sinf(wave_a) / ch * 6.0f;
    float hue0 = gk_hash(seed ^ 0x592u) + t * 0.00003f;
    float col[3], c2[3];
    for (j = 0; j < GH592; j++)
        for (i = 0; i < GW592; i++) {
            float x = nx592[j][i], y = ny592[j][i];
            float ph = x * kx + y * ky - t * 0.02f + gk_hash((uint32_t)(i * 31 + j * 977) ^ seed) * 1.5f;
            float tw = 0.5f + 0.5f * sinf(ph);
            tw = tw * tw * tw;
            float hue = hue0 + 0.25f * (x / cw + y / ch) * 0.5f + 0.05f * tw;
            sk_col(pal, sk_hidx(base592, hue), 0.35f * tw, 0.5f, 0.15f + 0.85f * tw, col);
            gk_dot(&g592, x, y, col, (0.9f + 0.9f * tw) * sc, (3.0f + 3.0f * tw) * sc, 0.4f);
            /* threads to right, down-left, down-right neighbours */
            int k;
            for (k = 0; k < 3; k++) {
                int i2 = i, j2 = j;
                if (k == 0) i2 = i + 1;
                else { j2 = j + 1; i2 = (j & 1) ? i + (k - 1) : i - 1 + (k - 1); }
                if (i2 < 0 || i2 >= GW592 || j2 >= GH592) continue;
                float ph2 = nx592[j2][i2] * kx + ny592[j2][i2] * ky - t * 0.02f;
                float tw2 = 0.5f + 0.5f * sinf(ph2); tw2 = tw2 * tw2 * tw2;
                float m = 0.06f + 0.14f * (tw + tw2) * 0.5f;
                c2[0] = col[0] * m; c2[1] = col[1] * m; c2[2] = col[2] * m;
                sk_line(&g592, x, y, nx592[j2][i2], ny592[j2][i2], 1.1f * sc, c2);
            }
        }
    gk_present(&g592, fb, w, h);
}
