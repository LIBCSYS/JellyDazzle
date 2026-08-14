#!/usr/bin/env python3
# gen_tables.py v2 — builds palette.bin (6 schemes), sintab.bin, shapes.bin
# Run:  python3 gen_tables.py
#
# palette.bin : 6 schemes x 32768 x u32 ARGB  (786,432 bytes)
#               draw.s crossfades scheme k -> k+1 on a slow clock
# sintab.bin  : 256 x i32 Q14 sine            (1,024 bytes)
# shapes.bin  : 5 shapes x 256x256 x i16 SDF  (655,360 bytes)
#               order: heart, diamond, club, spade, star
#               value = signed distance in grid px (<0 inside), y-down rows,
#               shape half-height ~= 100 grid px, center at (128,128)

import math, struct, colorsys
import numpy as np

N = 32768

# ---------------- six color schemes ----------------
# (position, hue, sat, val, shimmer) — same sampler as v1
SCHEMES = {}  # generated analytically below: SIX FULL-SPECTRUM RAINBOWS
# (J: "we have 32k colors, lets fucking use them") — every scheme contains
# the entire hue wheel; they differ in character, not in gamut.
#   name      hue cycles, hue phase, sat base, sat wave, val base, val wave
RAINBOWS = {
    'vivid':   (6, 0.00, 0.95, 0.05, 0.72, 0.25),
    'neon':    (8, 0.15, 1.00, 0.00, 0.80, 0.20),
    'pastel':  (5, 0.40, 0.50, 0.15, 0.90, 0.10),
    'deep':    (5, 0.60, 0.90, 0.10, 0.48, 0.30),
    'sunset':  (7, 0.85, 0.85, 0.15, 0.65, 0.30),
    'ocean':   (6, 0.50, 0.78, 0.18, 0.60, 0.30),
}
# 5-8 hue cycles across 32768: even a NARROW calm-moment index window
# (~2000 idx) spans half a rainbow — no more monochrome walls, ever.
ORDER = ['vivid', 'neon', 'pastel', 'deep', 'sunset', 'ocean']

def hlerp(a, b, t):
    d = (b - a + 0.5) % 1.0 - 0.5
    return (a + d * t) % 1.0

def sample(keys, p):
    for k in range(len(keys) - 1):
        p0, *A = keys[k]; p1, *B = keys[k + 1]
        if p0 <= p <= p1:
            t = (p - p0) / (p1 - p0); t = t * t * (3 - 2 * t)
            return (hlerp(A[0], B[0], t), A[1] + (B[1] - A[1]) * t,
                    A[2] + (B[2] - A[2]) * t, A[3] + (B[3] - A[3]) * t)
    return keys[-1][1:]

with open('palette.bin', 'wb') as f:
    for name in ORDER:
        cyc, ph, sb, sw, vb, vw = RAINBOWS[name]
        for i in range(N):
            p = i / N
            fr = p * 2 * math.pi
            h = (p * cyc + ph) % 1.0                      # FULL hue wheel
            s = sb + sw * math.sin(fr * 5)
            v = vb + vw * math.sin(fr * 7 + 1.3)          # broad soft waves
            v += 0.08 * math.sin(fr * 17)                 # mild sheen only
            s = max(0.15, min(1.0, s))
            v = max(0.06, min(1.0, v))
            r, g, b = colorsys.hsv_to_rgb(h, s, v)
            f.write(struct.pack('<I', 0xFF000000 | (int(r*255) << 16)
                                     | (int(g*255) << 8) | int(b*255)))

# ---------------- Q14 sine table ----------------
with open('sintab.bin', 'wb') as f:
    for i in range(256):
        f.write(struct.pack('<i', round(math.sin(i * 2 * math.pi / 256) * 16384)))

# ---------------- shape SDFs ----------------
# Grid: 256x256, x right, y UP in math here; rows written top-down (y-down)
# so draw.s can index directly with screen coordinates. 100 px = 1.0 unit.
G = 256
lin = (np.arange(G) - 128.0) / 100.0      # -1.28 .. 1.27
X, Yd = np.meshgrid(lin, lin)             # Yd grows downward (row index)
Y = -Yd                                   # math y up

def heart_mask(X, Y):
    # classic implicit, lobes up point down, scaled to ~unit height
    x = X / 0.95; y = Y / 0.95 + 0.05
    return (x**2 + y**2 - 1)**3 - (x**2) * (y**3) < 0

def diamond_mask(X, Y):
    return np.abs(X) / 0.80 + np.abs(Y) / 1.05 < 1

def club_mask(X, Y):
    r = 0.42
    c = ((X)**2 + (Y - 0.45)**2 < r*r) \
      | ((X + 0.42)**2 + (Y + 0.02)**2 < r*r) \
      | ((X - 0.42)**2 + (Y + 0.02)**2 < r*r) \
      | ((X)**2 + (Y - 0.10)**2 < 0.20**2)    # filler: no pinhole where lobes meet
    stem = (np.abs(X) < 0.10 * (1.0 + (-Y - 0.1) * 1.6)) & (Y < -0.10) & (Y > -1.05)
    return c | stem

def spade_mask(X, Y):
    body = heart_mask(X, -Y - 0.12)       # heart flipped: point up
    stem = (np.abs(X) < 0.10 * (1.0 + (-Y - 0.35) * 1.6)) & (Y < -0.35) & (Y > -1.05)
    return body | stem

def star_mask(X, Y):
    theta = np.arctan2(X, Y)              # 0 at top, point up
    a = (theta * 5 / (2 * np.pi)) % 1.0
    tri = np.abs(a - 0.5) * 2             # 1 at points, 0 between
    rmax = 0.45 + (1.10 - 0.45) * tri**2.2
    return np.sqrt(X**2 + Y**2) < rmax

def sdf(mask):
    """signed distance (grid px) to mask boundary, brute force numpy"""
    m = mask.astype(bool)
    er = m.copy()
    er[1:, :] &= m[:-1, :]; er[:-1, :] &= m[1:, :]
    er[:, 1:] &= m[:, :-1]; er[:, :-1] &= m[:, 1:]
    edge = m & ~er
    by, bx = np.nonzero(edge)
    if len(bx) == 0:
        return np.full((G, G), 300, np.int16)
    pts = np.stack([bx, by], 1).astype(np.float32)
    yy, xx = np.mgrid[0:G, 0:G]
    q = np.stack([xx.ravel(), yy.ravel()], 1).astype(np.float32)
    d = np.empty(G * G, np.float32)
    for i in range(0, G * G, 8192):                     # chunked min-dist
        blk = q[i:i+8192]
        dd = ((blk[:, None, :] - pts[None, :, :]) ** 2).sum(-1)
        d[i:i+8192] = np.sqrt(dd.min(1))
    d = d.reshape(G, G)
    d[m] *= -1                                          # negative inside
    return np.clip(np.round(d), -32000, 32000).astype(np.int16)

shapes = [('heart', heart_mask), ('diamond', diamond_mask),
          ('club', club_mask), ('spade', spade_mask), ('star', star_mask)]

with open('shapes.bin', 'wb') as f:
    for name, fn in shapes:
        field = sdf(fn(X, Y))
        f.write(field.tobytes())          # int16 LE, row 0 = top (y-down)
        print(f'  shape {name:8s} sdf ok  (min {field.min()}, max {field.max()})')

import os
assert os.path.getsize('palette.bin') == 6 * 131072, 'palette size'
assert os.path.getsize('sintab.bin') == 1024
assert os.path.getsize('shapes.bin') == 5 * G * G * 2
print('palette.bin', os.path.getsize('palette.bin'), 'bytes  OK (6 schemes)')
print('sintab.bin    1024 bytes  OK')
print('shapes.bin ', os.path.getsize('shapes.bin'), 'bytes  OK (5 shapes)')
