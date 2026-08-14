#!/usr/bin/env python3
"""024 Munch Frost — rotating munching-squares XOR lattice, rainbow Sierpinski frost."""
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
    rot = 0.0015 * tt                          # slow frame rotation
    sc = 1.0 + 0.22 * np.sin(0.0025 * tt)      # gentle zoom breathing
    c, s = np.cos(rot), np.sin(rot)
    dx, dy = X - CX, Y - CY
    xi = (dx * c - dy * s) * sc
    yi = (dx * s + dy * c) * sc
    ix = xi.astype(np.int32) + 4096
    iy = yi.astype(np.int32) + 4096
    v = ((ix ^ iy) + int(0.6 * tt)) & 127      # munching XOR field, cycling
    u = (v / 127.0).astype(np.float32)
    hue = u + 0.0008 * tt                      # full wheel across the bands
    sat = 0.78 + 0.20 * np.cos(u * 6.2832) * 0.5
    val = 0.18 + 0.82 * u ** 0.75
    return _hsv(hue, np.clip(sat, 0.0, 1.0), val)


if __name__ == "__main__":
    frames = [render(t) for t in (0, 300, 700)]
    strip = np.concatenate(frames, axis=1)
    ppm = "/tmp/dzzle_024.ppm"
    with open(ppm, "wb") as fh:
        fh.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        fh.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out], check=True)
