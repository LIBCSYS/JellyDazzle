#!/usr/bin/env python3
"""027 Wedge Ripples — 8-fold kaleidoscope fold of off-axis ring interference."""
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
    dx, dy = X - CX, Y - CY
    a = np.arctan2(dy, dx)
    r = np.hypot(dx, dy)
    N = 8
    wedge = 2.0 * np.pi / N
    fa = np.abs(((a + 0.0035 * tt) % wedge) - 0.5 * wedge)   # fold + slow spin
    u = r * np.cos(fa)
    v = r * np.sin(fa)
    # two orbiting sources living in wedge space (off the fold axis)
    s1x = 62 + 26 * np.cos(0.007 * tt)
    s1y = 26 + 16 * np.sin(0.009 * tt)
    s2x = 18 + 10 * np.sin(0.006 * tt)
    s2y = 62 + 20 * np.sin(0.005 * tt + 1.0)
    d1 = np.hypot(u - s1x, v - s1y)
    d2 = np.hypot(u - s2x, v - s2y)
    f = np.sin(0.30 * d1 - 0.040 * tt) + np.sin(0.26 * d2 + 0.030 * tt)
    hue = 0.34 + 0.07 * f + 0.10 * np.sin(0.012 * r - 0.010 * tt) + 0.0004 * tt
    sat = np.clip(0.85 - 0.10 * np.abs(f) * 0.5, 0.0, 1.0)
    val = np.clip(0.20 + 0.60 * (0.5 + 0.25 * f) + 0.20 * np.cos(0.5 * (d1 - d2) * 0.15) ** 2, 0.0, 1.0)
    return _hsv(hue, sat, val)


if __name__ == "__main__":
    frames = [render(t) for t in (0, 300, 700)]
    strip = np.concatenate(frames, axis=1)
    ppm = "/tmp/dzzle_027.ppm"
    with open(ppm, "wb") as fh:
        fh.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        fh.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out], check=True)
