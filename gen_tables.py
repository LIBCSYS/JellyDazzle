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
SCHEMES = {
    'jewels': [   # the original materials ramp
        (0.00, 0.615, 0.97, 0.28, 0.05), (0.10, 0.760, 0.95, 0.42, 0.08),
        (0.20, 0.380, 0.97, 0.36, 0.06), (0.30, 0.130, 0.80, 0.97, 0.30),
        (0.40, 0.070, 0.85, 0.58, 0.26), (0.50, 0.520, 0.65, 0.85, 0.32),
        (0.60, 0.580, 0.08, 0.94, 0.36), (0.70, 0.950, 0.34, 0.98, 0.06),
        (0.80, 0.450, 0.32, 0.97, 0.05), (0.90, 0.720, 0.36, 0.97, 0.06),
        (1.00, 0.615, 0.97, 0.28, 0.05)],
    'ember': [    # DARK RED heart with teal + gold counterpoints
        (0.00, 0.990, 0.90, 0.12, 0.03), (0.15, 0.985, 0.97, 0.38, 0.06),
        (0.30, 0.030, 0.90, 0.72, 0.16), (0.45, 0.510, 0.70, 0.80, 0.20),
        (0.60, 0.985, 0.95, 0.50, 0.10), (0.75, 0.120, 0.75, 0.97, 0.28),
        (0.88, 0.760, 0.60, 0.55, 0.10), (1.00, 0.990, 0.90, 0.12, 0.03)],
    'royal': [    # PURPLE court with gold, emerald and rose guests
        (0.00, 0.740, 0.98, 0.22, 0.04), (0.16, 0.760, 0.95, 0.50, 0.10),
        (0.32, 0.130, 0.78, 0.95, 0.30), (0.48, 0.820, 0.80, 0.75, 0.14),
        (0.62, 0.380, 0.80, 0.60, 0.12), (0.76, 0.900, 0.55, 0.90, 0.10),
        (0.88, 0.720, 0.90, 0.45, 0.10), (1.00, 0.740, 0.98, 0.22, 0.04)],
    'gilded': [   # GOLD hall hung with sapphire and ruby
        (0.00, 0.090, 0.90, 0.28, 0.08), (0.18, 0.120, 0.85, 0.75, 0.24),
        (0.36, 0.610, 0.85, 0.55, 0.14), (0.52, 0.130, 0.75, 0.96, 0.34),
        (0.68, 0.960, 0.80, 0.60, 0.12), (0.84, 0.140, 0.50, 1.00, 0.28),
        (1.00, 0.090, 0.90, 0.28, 0.08)],
    'ice': [      # LIGHT BLUE with coral dawn and violet dusk
        (0.00, 0.620, 0.95, 0.25, 0.04), (0.16, 0.580, 0.70, 0.70, 0.14),
        (0.34, 0.050, 0.60, 0.90, 0.18), (0.50, 0.540, 0.40, 0.98, 0.28),
        (0.66, 0.760, 0.55, 0.65, 0.12), (0.82, 0.560, 0.65, 0.85, 0.16),
        (1.00, 0.620, 0.95, 0.25, 0.04)],
    'spring': [   # LIGHT GREEN meadow with lilac and honey
        (0.00, 0.360, 0.95, 0.22, 0.04), (0.16, 0.330, 0.80, 0.60, 0.12),
        (0.34, 0.780, 0.45, 0.85, 0.14), (0.50, 0.290, 0.40, 0.98, 0.24),
        (0.66, 0.120, 0.60, 0.95, 0.20), (0.82, 0.400, 0.60, 0.65, 0.12),
        (1.00, 0.360, 0.95, 0.22, 0.04)],
}
ORDER = ['jewels', 'ember', 'royal', 'gilded', 'ice', 'spring']

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
        keys = SCHEMES[name]
        for i in range(N):
            h, s, v, sh = sample(keys, i / N)
            fr = i * 2 * math.pi / N
            # broad, soft texture: high-frequency shimmer made motion
            # read as marching noise — VGA-style wide ramps relax
            v += sh * 0.35 * (0.6 * math.sin(fr * 10) + 0.4 * math.sin(fr * 27))
            v += 0.10 * max(0.0, math.sin(fr * 6)) ** 5         # gentle crests
            v -= 0.07 * max(0.0, math.sin(fr * 6 + 2.1)) ** 5   # gentle troughs
            v = max(0.02, min(1.0, v))
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
