/* JellyDazzle pattern plug-in contract.
 * A pattern paints fb (w*h ARGB u32, row-major). frame = global frame
 * counter; sl = segment-local frame 0..2047 (~34s @60fps) — accumulator
 * patterns clear their canvas when sl==0 and build until segment end;
 * repaint patterns ignore sl. seed = per-segment random (stable within
 * the segment). pal = 32768-entry ARGB palette, already scheme-blended
 * and crossfading slowly — index it with (anything & 0x7FFF).
 * Motion law: SLOW and SMOOTH. Nothing may strobe. */
#include <stdint.h>
#define JD_PAL_MASK 0x7FFF
typedef void (*jd_pattern_fn)(uint32_t *fb, int w, int h,
                              int frame, int sl, uint32_t seed,
                              const uint32_t *pal);
