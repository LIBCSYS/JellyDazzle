#!/usr/bin/env python3
"""021 Ripple Duet — two orbiting wave sources, two-slit pond interference."""
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
    # two sources on counter-rotating ellipses around the center
    a1 = 0.009 * tt
    a2 = -0.007 * tt + 2.1
    x1 = CX + 70 * np.cos(a1)
    y1 = CY + 50 * np.sin(a1)
    x2 = CX + 70 * np.cos(a2)
    y2 = CY + 50 * np.sin(a2)
    d1 = np.hypot(X - x1, Y - y1)
    d2 = np.hypot(X - x2, Y - y2)
    k = 0.22
    w1 = np.sin(k * d1 - 0.045 * tt)
    w2 = np.sin(k * d2 - 0.038 * tt)
    f = w1 + w2                            # -2..2 fringe field
    env = np.cos(0.5 * k * (d1 - d2))      # hyperbolic beat envelope
    hue = 0.52 + 0.10 * env + 0.06 * f + 0.0004 * tt
    sat = np.clip(0.88 - 0.10 * np.abs(f) * 0.5, 0.0, 1.0)
    val = np.clip(0.24 + 0.55 * (0.5 + 0.25 * f) + 0.18 * env * env, 0.0, 1.0)
    return _hsv(hue, sat, val)


if __name__ == "__main__":
    frames = [render(t) for t in (0, 300, 700)]
    strip = np.concatenate(frames, axis=1)
    ppm = "/tmp/dzzle_021.ppm"
    with open(ppm, "wb") as fh:
        fh.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        fh.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out], check=True)
