import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)


def pal(v):
    # blue/cyan/yellow/green family (R13 gear-flower reference)
    a = np.array([0.45, 0.50, 0.45], np.float32)
    b = np.array([0.45, 0.50, 0.45], np.float32)
    c = np.array([1.0, 1.0, 1.0], np.float32)
    d = np.array([0.35, 0.55, 0.80], np.float32)
    return np.clip(a + b * np.cos(2 * np.pi * (c * v[..., None] + d)), 0, 1)


def render(t):
    t = float(t)
    # ground: slow-drifting diagonal stripes
    s = (_xx + _yy) * 0.5
    ground = 0.5 + 0.5 * np.sin(s * 0.35 - t * 0.008)
    hueg = 0.62 + 0.05 * np.sin(s * 0.04 + t * 0.002)
    img = pal(hueg) * (0.18 + 0.22 * ground)[..., None]

    # 2x2 gear-flower rosettes
    for i, (cx, cy) in enumerate([(80.0, 60.0), (240.0, 60.0),
                                  (80.0, 180.0), (240.0, 180.0)]):
        dx = _xx - cx
        dy = _yy - cy
        r = np.hypot(dx, dy)
        th = np.arctan2(dy, dx)
        rot = t * 0.006 * (1.0 if i % 2 else -1.0) + i * 0.7
        R = 44.0 + 5.0 * np.sin(16.0 * (th + rot))       # 16 gear teeth
        inside = r < R
        # seed core: concentric dot rings (radial x angular interference)
        seed = 0.5 + 0.5 * np.sin(r * 0.55 - t * 0.012) * np.sin(8.0 * (th - rot * 2.0))
        hue = 0.13 + 0.05 * np.sin(t * 0.004 + i) + seed * 0.12
        col = pal(hue) * (0.40 + 0.60 * seed)[..., None]
        img = np.where(inside[..., None], col, img)
        # bright rim glow on the toothed edge
        rim = np.clip(1.0 - np.abs(r - R) / 2.5, 0, 1)
        img = img + rim[..., None] * np.array([0.5, 0.5, 0.2], np.float32)

    # mirrored motif column on the center vertical (small diamonds)
    dcol = np.abs(_xx - 160.0)
    dm = np.abs(np.mod(_yy + t * 0.05, 30.0) - 15.0) + dcol
    colm = np.clip(1.0 - dm / 9.0, 0, 1)
    img = img + colm[..., None] * np.array([0.15, 0.5, 0.55], np.float32)
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os
    import subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_084.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
