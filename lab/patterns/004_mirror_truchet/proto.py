import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
X = _xx - W / 2
Y = _yy - H / 2
RSC = np.hypot(X, Y)

# ribbon palette (per-tile hues) and ground palette (deep radial)
RA = np.array([0.55, 0.5, 0.5]); RB = np.array([0.45, 0.5, 0.5])
RC = np.array([1.0, 1.0, 1.0]);  RD = np.array([0.00, 0.33, 0.67])
GA = np.array([0.12, 0.08, 0.20]); GB = np.array([0.10, 0.08, 0.16])
GC = np.array([1.0, 1.0, 1.0]);    GD = np.array([0.60, 0.55, 0.30])


def cpal(v, a, b, c, d):
    return np.clip(a + b * np.cos(2 * np.pi * (c * v[..., None] + d)), 0, 1)


def render(t):
    t = float(t)
    rot = t * 0.0020
    c, s = np.cos(rot), np.sin(rot)
    x = X * c - Y * s
    y = X * s + Y * c
    # 8-fold dihedral fold: abs both axes + diagonal swap
    x = np.abs(x)
    y = np.abs(y)
    fx = np.maximum(x, y)
    fy = np.minimum(x, y)
    # scroll the folded plane, then Truchet-tile it
    u0 = fx + t * 0.22
    v0 = fy + t * 0.13
    L = 42.0
    tx = np.floor(u0 / L)
    ty = np.floor(v0 / L)
    u = u0 - tx * L
    v = v0 - ty * L
    h = np.sin(tx * 12.9898 + ty * 78.233) * 43758.5453
    h = h - np.floor(h)
    u = np.where(h < 0.5, u, L - u)                 # tile orientation
    d1 = np.hypot(u, v)
    d2 = np.hypot(L - u, L - v)
    Rr = L / 2
    d = np.minimum(np.abs(d1 - Rr), np.abs(d2 - Rr))
    ribbon = np.clip(1.0 - d / 7.0, 0.0, 1.0)
    ribbon = ribbon * ribbon * (3 - 2 * ribbon)
    hue = tx * 0.11 + ty * 0.07 + t * 0.0012
    ink = cpal(hue, RA, RB, RC, RD)
    ground = cpal(RSC * 0.004 + t * 0.0006, GA, GB, GC, GD)
    out = ground * (1 - ribbon[..., None]) + ink * ribbon[..., None]
    return (np.clip(out, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_004.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
