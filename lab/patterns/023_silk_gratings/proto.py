#!/usr/bin/env python3
"""023 Silk Gratings — two near-parallel line gratings beating into rose silk moire."""
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
    # two far off-screen ring sources -> gently curved "gratings" whose fringes beat
    ax = CX - 420 + 60 * np.sin(0.0035 * tt)
    ay = CY + 120 * np.sin(0.0028 * tt)
    bx = CX + 420 - 60 * np.sin(0.0030 * tt)
    by = CY - 120 * np.sin(0.0026 * tt + 0.9)
    da = np.hypot(X - ax, Y - ay)
    db = np.hypot(X - bx, Y - by)
    f1, f2 = 0.55, 0.61                       # close spatial frequencies -> beat
    g1 = np.sin(f1 * da + 0.030 * tt)
    g2 = np.sin(f2 * db - 0.024 * tt)
    carrier = g1 * g2                         # fine weave
    env = np.cos(0.5 * (f1 * da - f2 * db) + 0.010 * tt)  # broad curved beat bands
    hue = 0.86 + 0.11 * env + 0.04 * carrier + 0.0003 * tt
    sat = np.clip(0.70 + 0.25 * env * env, 0.0, 1.0)
    val = np.clip(0.22 + 0.48 * (0.5 + 0.5 * carrier) + 0.28 * env * env, 0.0, 1.0)
    return _hsv(hue, sat, val)


if __name__ == "__main__":
    frames = [render(t) for t in (0, 300, 700)]
    strip = np.concatenate(frames, axis=1)
    ppm = "/tmp/dzzle_023.ppm"
    with open(ppm, "wb") as fh:
        fh.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        fh.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out], check=True)
