#!/usr/bin/env python3
"""026 Spoke Moire — counter-rotating spoke fans beat into a rainbow pinwheel."""
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
    # counter-rotating spoke gratings, slight spiral twist on each
    g1 = np.sin(9.0 * a + 0.020 * r + 0.012 * tt)
    g2 = np.sin(11.0 * a - 0.016 * r - 0.010 * tt)
    ring = np.sin(0.16 * r - 0.045 * tt)          # breathing radial rings
    f = 0.5 * (g1 + g2)                            # 2-lobe angular beat envelope
    hue = a / 6.2832 + 0.10 * ring + 0.0006 * tt   # wheel around the center
    sat = np.clip(0.80 + 0.18 * f, 0.0, 1.0)
    val = 0.15 + 0.85 * np.clip((0.5 + 0.5 * f) * (0.55 + 0.45 * ring), 0.0, 1.0)
    return _hsv(hue, sat, val)


if __name__ == "__main__":
    frames = [render(t) for t in (0, 300, 700)]
    strip = np.concatenate(frames, axis=1)
    ppm = "/tmp/dzzle_026.ppm"
    with open(ppm, "wb") as fh:
        fh.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        fh.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out], check=True)
