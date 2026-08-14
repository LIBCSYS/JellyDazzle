#!/usr/bin/env python3
"""030 Octa Facets — octagonal-norm ring interference, faceted ice-violet crystal."""
import numpy as np, os, subprocess

W, H = 320, 240
_Y, _X = np.mgrid[0:H, 0:W]
X = _X.astype(np.float32)
Y = _Y.astype(np.float32)
CX, CY = W / 2.0, H / 2.0


def _hsv(h, s, v):
    h = (h % 1.0) * 6.0
    c = v * s
    x = c * (1.0 - np.abs(h % 2.0 - 1.0))
    m = v - c
    z = np.zeros_like(h)
    r = np.select([h < 1, h < 2, h < 3, h < 4, h < 5], [c, x, z, z, x], c)
    g = np.select([h < 1, h < 2, h < 3, h < 4, h < 5], [x, c, c, x, z], z)
    b = np.select([h < 1, h < 2, h < 3, h < 4, h < 5], [z, z, x, c, c], x)
    rgb = np.stack([r + m, g + m, b + m], axis=-1)
    return (np.clip(rgb, 0.0, 1.0) * 255.0).astype(np.uint8)


def _octd(dx, dy):
    ax, ay = np.abs(dx), np.abs(dy)
    return np.maximum(np.maximum(ax, ay), 0.70711 * (ax + ay))


def _tri(z):
    return np.abs(z % 2.0 - 1.0)


def render(t):
    tt = float(t)
    a = 0.005 * tt
    x1 = CX + 58 * np.cos(a)
    y1 = CY + 40 * np.sin(0.8 * a)
    x2 = CX - 58 * np.cos(0.9 * a)
    y2 = CY - 40 * np.sin(a + 1.3)
    d1 = _octd(X - x1, Y - y1)
    d2 = _octd(X - x2, Y - y2)
    r1 = _tri(d1 * 0.055 - 0.010 * tt)          # octagon ring trains, 0..1
    r2 = _tri(d2 * 0.055 + 0.008 * tt)
    f = r1 + r2                                  # 0..2 faceted interference
    ridge = _tri(f)                              # peaks where trains agree
    hue = 0.60 + 0.15 * (f - 1.0) + 0.0005 * tt  # ice blue <-> violet
    sat = np.clip(0.55 + 0.40 * np.abs(f - 1.0) + 0.15 * ridge, 0.0, 1.0)
    val = np.clip(0.14 + 0.86 * ridge ** 1.3, 0.0, 1.0)
    return _hsv(hue, sat, val)


if __name__ == "__main__":
    frames = [render(t) for t in (0, 300, 700)]
    strip = np.concatenate(frames, axis=1)
    ppm = "/tmp/dzzle_030.ppm"
    with open(ppm, "wb") as fh:
        fh.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        fh.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out], check=True)
