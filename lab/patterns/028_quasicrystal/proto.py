#!/usr/bin/env python3
"""028 Quasicrystal — seven plane waves superpose into a turning aperiodic peacock lattice."""
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
    N = 7
    rot = 0.0012 * tt
    k = 0.30
    dx, dy = X - CX, Y - CY
    f = np.zeros((H, W), np.float32)
    for i in range(N):
        th = rot + i * np.pi / N
        u = dx * np.cos(th) + dy * np.sin(th)
        sgn = 1.0 if (i % 2 == 0) else -1.0
        f += np.cos(k * u + sgn * 0.028 * tt)
    f *= (1.0 / N)
    g = np.tanh(2.6 * f)                      # contrast: bold plateaus, thin walls
    hue = 0.52 + 0.17 * g + 0.0005 * tt       # peacock blue-green-violet
    sat = np.clip(0.85 - 0.20 * g * g, 0.0, 1.0)
    val = np.clip(0.14 + 0.86 * (0.5 + 0.5 * g), 0.0, 1.0)
    return _hsv(hue, sat, val)


if __name__ == "__main__":
    frames = [render(t) for t in (0, 300, 700)]
    strip = np.concatenate(frames, axis=1)
    ppm = "/tmp/dzzle_028.ppm"
    with open(ppm, "wb") as fh:
        fh.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        fh.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out], check=True)
