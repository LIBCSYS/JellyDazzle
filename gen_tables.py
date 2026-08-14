#!/usr/bin/env python3
# gen_tables.py — builds palette.bin and sintab.bin for dazzle64.
# Run:  python3 gen_tables.py
# Edit the keyframes below to change the look; no assembly edits needed.

import math, struct, colorsys

# ---------------- materials palette ----------------
# (position, hue, sat, val, shimmer)
KEYS = [
    (0.00, 0.615, 0.97, 0.28, 0.05),  # abyssal sapphire
    (0.10, 0.760, 0.95, 0.42, 0.08),  # royal purple
    (0.20, 0.380, 0.97, 0.36, 0.06),  # deep emerald
    (0.30, 0.130, 0.80, 0.97, 0.30),  # blazing gold
    (0.40, 0.070, 0.85, 0.58, 0.26),  # dark copper
    (0.50, 0.520, 0.65, 0.85, 0.32),  # teal chrome
    (0.60, 0.580, 0.08, 0.94, 0.36),  # brushed silver
    (0.70, 0.950, 0.34, 0.98, 0.06),  # pastel rose
    (0.80, 0.450, 0.32, 0.97, 0.05),  # pastel mint
    (0.90, 0.720, 0.36, 0.97, 0.06),  # pastel lavender
    (1.00, 0.615, 0.97, 0.28, 0.05),  # wrap
]
N = 32768

def hlerp(a, b, t):
    d = (b - a + 0.5) % 1.0 - 0.5
    return (a + d * t) % 1.0

def sample(p):
    for k in range(len(KEYS) - 1):
        p0, *A = KEYS[k]; p1, *B = KEYS[k + 1]
        if p0 <= p <= p1:
            t = (p - p0) / (p1 - p0); t = t * t * (3 - 2 * t)
            return (hlerp(A[0], B[0], t), A[1] + (B[1] - A[1]) * t,
                    A[2] + (B[2] - A[2]) * t, A[3] + (B[3] - A[3]) * t)
    return KEYS[-1][1:]

with open('palette.bin', 'wb') as f:
    for i in range(N):
        h, s, v, sh = sample(i / N)
        fr = i * 2 * math.pi / N
        v += sh * (0.55 * math.sin(fr * 48) + 0.45 * math.sin(fr * 131))
        v += 0.38 * max(0.0, math.sin(fr * 24)) ** 9      # chrome filaments
        v -= 0.22 * max(0.0, math.sin(fr * 24 + 2.1)) ** 7  # dark grooves
        v = max(0.02, min(1.0, v))
        r, g, b = colorsys.hsv_to_rgb(h, s, v)
        f.write(struct.pack('<I', 0xFF000000 | (int(r*255) << 16)
                                 | (int(g*255) << 8) | int(b*255)))

# ---------------- Q14 sine table ----------------
with open('sintab.bin', 'wb') as f:
    for i in range(256):
        f.write(struct.pack('<i', round(math.sin(i * 2 * math.pi / 256) * 16384)))

import os
assert os.path.getsize('palette.bin') == 131072
assert os.path.getsize('sintab.bin') == 1024
print('palette.bin 131072 bytes  OK')
print('sintab.bin    1024 bytes  OK')
