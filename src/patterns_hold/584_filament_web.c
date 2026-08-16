/* 584 Filament Web — a few dozen anchor points wander slowly; whenever two
 * are near enough a faint filament joins them, and glints crawl along the
 * filaments.  Threads fade in and out as anchors drift apart.  Hues per
 * anchor, blended along each thread.  Repaint pattern. */
#include "_spark572.h"

#define NA584 34

static gk g584;
static float ax584[NA584], ay584[NA584], ah584[NA584], aph584[NA584];
static uint32_t bs584 = 0xFFFFFFFFu;
static int base584;

void pattern_584(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, j;
    gk_setup(&g584, w, h);
    gk_clear(&g584);
    float cw = (float)g584.cw, ch = (float)g584.ch, sc = g584.sc;
    if (seed != bs584) {
        base584 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NA584; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 4099);
            ah584[i] = gk_hash(seed ^ 0x584u) + gk_hash(r + 4u) * 0.35f;
            aph584[i] = gk_hash(r + 5u) * 100.0f;
        }
        bs584 = seed;
    }
    float t = (float)frame;
    for (i = 0; i < NA584; i++) {
        ax584[i] = cw * (0.5f + 1.7f * (gk_noise1(t * 0.0025f + aph584[i], (uint32_t)i * 3u + 1u) - 0.5f));
        ay584[i] = ch * (0.5f + 1.7f * (gk_noise1(t * 0.0025f + aph584[i] * 1.3f, (uint32_t)i * 3u + 2u) - 0.5f));
    }
    float D = ch * 0.34f, D2 = D * D;
    float ca[3], cb[3], cm[3];
    for (i = 0; i < NA584; i++) {
        sk_col(pal, sk_hidx(base584, ah584[i] + t * 0.00004f), 0.2f, 0.5f, 1.0f, ca);
        for (j = i + 1; j < NA584; j++) {
            float dx = ax584[j] - ax584[i], dy = ay584[j] - ay584[i];
            float d2 = dx * dx + dy * dy;
            if (d2 > D2) continue;
            float d = sqrtf(d2);
            float f = 1.0f - d / D;               /* fades as they part */
            f = f * f * (3.0f - 2.0f * f);
            sk_col(pal, sk_hidx(base584, ah584[j] + t * 0.00004f), 0.2f, 0.5f, 1.0f, cb);
            int k;
            for (k = 0; k < 3; k++) cm[k] = (ca[k] + cb[k]) * 0.5f * f * 0.28f;
            sk_line(&g584, ax584[i], ay584[i], ax584[j], ay584[j], 1.2f * sc, cm);
            /* a glint crawling along the thread */
            float u = gk_noise1(t * 0.006f + (float)(i * 7 + j * 3), 55u);
            u = 0.5f + 0.5f * sinf(t * 0.012f + (float)(i * 13 + j * 5) * 0.7f) * (0.4f + 0.6f * u);
            float gx = ax584[i] + dx * u, gy = ay584[i] + dy * u;
            for (k = 0; k < 3; k++) cm[k] = (ca[k] * (1.0f - u) + cb[k] * u) * f * 0.9f;
            gk_dot(&g584, gx, gy, cm, 1.2f * sc, 4.0f * sc, 0.4f);
        }
        /* the anchor itself: soft knot */
        float k2[3] = { ca[0] * 0.7f, ca[1] * 0.7f, ca[2] * 0.7f };
        gk_dot(&g584, ax584[i], ay584[i], k2, 1.6f * sc, 5.0f * sc, 0.4f);
    }
    gk_present(&g584, fb, w, h);
}
