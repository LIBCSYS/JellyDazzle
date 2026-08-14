#!/usr/bin/env python3
"""022 XOR Rings — hard-fringe XOR moire of two drifting ring fields, indigo/gold."""
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


def render(t):
    tt = float(t)
    a1 = 0.006 * tt
    a2 = -0.005 * tt + 1.7
    x1 = CX + 55 * np.cos(a1)
    y1 = CY + 40 * np.sin(1.3 * a1)
    x2 = CX - 55 * np.cos(a2)
    y2 = CY - 40 * np.sin(a2)
    d1 = (np.hypot(X - x1, Y - y1) * 1.6 + 0.35 * tt).astype(np.int32)
    d2 = (np.hypot(X - x2, Y - y2) * 1.6 - 0.28 * tt).astype(np.int32)
    v = (d1 ^ d2) & 63                      # 0..63 hard XOR fringes
    u = (v / 63.0).astype(np.float32)
    hue = 0.62 - 0.55 * u ** 1.3 + 0.0005 * tt
    sat = 0.92 - 0.35 * u * u
    val = 0.14 + 0.86 * u
    return _hsv(hue, sat, val)


if __name__ == "__main__":
    frames = [render(t) for t in (0, 300, 700)]
    strip = np.concatenate(frames, axis=1)
    ppm = "/tmp/dzzle_022.ppm"
    with open(ppm, "wb") as fh:
        fh.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        fh.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out], check=True)
