/* asm_visit_audit.c — do the 24 asm ground modes look the same on every visit?
 * Renders each mode at the same segment-local frame in two different 2048-frame
 * segments and compares EDGE maps (|luma gradient| > T), so a palette change is
 * ignored and only geometry counts. IoU ~1 = same picture every visit. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

extern uint32_t g_mode;
extern void draw_frame(uint32_t *, int, int, int);
#define W 320
#define H 240
static uint32_t a[W * H], b[W * H];
static uint8_t ea[W * H], eb[W * H];

static int lum(uint32_t c) { return ((c >> 16 & 255) * 54 + (c >> 8 & 255) * 183 + (c & 255) * 19) >> 8; }
static void edges(const uint32_t *p, uint8_t *e)
{
    for (int y = 0; y < H - 1; y++)
        for (int x = 0; x < W - 1; x++) {
            int l = lum(p[y * W + x]);
            int g = abs(l - lum(p[y * W + x + 1])) + abs(l - lum(p[(y + 1) * W + x]));
            e[y * W + x] = g > 24;
        }
}
static void at(int mode, int frame, uint32_t *buf)
{
    g_mode = (uint32_t)mode;
    for (int f = frame - 30; f <= frame; f++) draw_frame(buf, W, H, f);  /* accumulators */
}
int main(void)
{
    printf("mode,iou_mean,iou_min,iou_max,samples\n");
    for (int m = 0; m < 24; m++) {
        double s = 0, mn = 1, mx = 0; int n = 0;
        for (int k = 0; k < 4; k++) {
            int local = 600 + k * 300;
            int s1 = 2048 * (11 + 7 * k), s2 = 2048 * (53 + 13 * k);
            at(m, s1 + local, a); at(m, s2 + local, b);
            edges(a, ea); edges(b, eb);
            long i = 0, u = 0;
            for (int p = 0; p < W * H; p++) { i += ea[p] & eb[p]; u += ea[p] | eb[p]; }
            if (u < 200) continue;          /* blank sample: no geometry to compare */
            double iou = (double)i / u;
            s += iou; n++; if (iou < mn) mn = iou; if (iou > mx) mx = iou;
        }
        printf("%d,%.3f,%.3f,%.3f,%d\n", m, n ? s / n : -1, mn, mx, n);
    }
    return 0;
}
