#!/usr/bin/env python3
"""025 Ember Triad — three detuned ring sources on a turning triangle, sunset beats."""
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
    base = 0.004 * tt
    ks = (0.30, 0.315, 0.285)                  # detuned ring frequencies -> beats
    ws = (0.050, 0.042, 0.058)
    f = np.zeros((H, W), np.float32)
    for i in range(3):
        ang = base + i * 2.0943951
        sx = CX + 62 * np.cos(ang)
        sy = CY + 46 * np.sin(ang)
        d = np.hypot(X - sx, Y - sy)
        f += np.sin(ks[i] * d - ws[i] * tt)
    f *= (1.0 / 3.0)                           # -1..1
    hue = 0.03 + 0.12 * f + 0.03 * np.sin(0.003 * tt)   # wine troughs -> gold crests
    sat = np.clip(0.95 - 0.10 * f * f, 0.0, 1.0)
    val = 0.24 + 0.76 * np.clip(0.5 + 0.55 * f, 0.0, 1.0) ** 1.15
    return _hsv(hue, sat, val)


if __name__ == "__main__":
    frames = [render(t) for t in (0, 300, 700)]
    strip = np.concatenate(frames, axis=1)
    ppm = "/tmp/dzzle_025.ppm"
    with open(ppm, "wb") as fh:
        fh.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        fh.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out], check=True)
