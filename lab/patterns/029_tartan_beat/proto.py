#!/usr/bin/env python3
"""029 Tartan Beat — two square lattices at detuned scale/angle weave a jade-copper moire plaid."""
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


def _grid(ang, freq, ph):
    dx, dy = X - CX, Y - CY
    u = dx * np.cos(ang) - dy * np.sin(ang)
    v = dx * np.sin(ang) + dy * np.cos(ang)
    return np.cos(freq * u + ph) + np.cos(freq * v - ph)


def render(t):
    tt = float(t)
    # bounded angle swing keeps the moire cells large (divergence -> mush)
    a1 = 0.16 * np.sin(0.0025 * tt)
    a2 = -0.14 * np.sin(0.0020 * tt) + 0.10
    f2 = 0.395 * (1.0 + 0.05 * np.sin(0.003 * tt))
    g1 = _grid(a1, 0.36, 0.020 * tt)
    g2 = _grid(a2, f2, -0.017 * tt)
    f = g1 + g2                                   # -4..4 tartan moire
    w = g1 * g2 * 0.25                            # weave sparkle
    hue = 0.235 + 0.175 * np.tanh(0.9 * f)        # copper <-> jade split
    sat = np.clip(0.85 - 0.18 * w, 0.0, 1.0)
    val = np.clip(0.14 + 0.72 * (np.abs(f) * 0.25) ** 1.1 + 0.20 * np.abs(w), 0.0, 1.0)
    return _hsv(hue, sat, val)


if __name__ == "__main__":
    frames = [render(t) for t in (0, 300, 700)]
    strip = np.concatenate(frames, axis=1)
    ppm = "/tmp/dzzle_029.ppm"
    with open(ppm, "wb") as fh:
        fh.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        fh.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out], check=True)
