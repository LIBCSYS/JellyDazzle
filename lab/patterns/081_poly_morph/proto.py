import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)


def pal(v):
    a = np.array([0.5, 0.5, 0.5], np.float32)
    b = np.array([0.5, 0.5, 0.5], np.float32)
    c = np.array([1.0, 1.0, 1.0], np.float32)
    d = np.array([0.00, 0.33, 0.67], np.float32)
    return np.clip(a + b * np.cos(2 * np.pi * (c * v[..., None] + d)), 0, 1)


def ngon_r(theta, n):
    # radius multiplier of a regular n-gon (circumradius 1) at angle theta
    seg = 2.0 * np.pi / n
    a = np.mod(theta, seg) - seg / 2
    return np.cos(np.pi / n) / np.cos(a)


def render(t):
    t = float(t)
    # ground: dim violet field with slow breathing rings
    dx = _xx - 160.0
    dy = _yy - 120.0
    r0 = np.hypot(dx, dy)
    field = 0.10 + 0.06 * np.sin(r0 * 0.05 - t * 0.010)
    img = field[..., None] * np.array([0.35, 0.22, 0.65], np.float32)

    # five morphing polygon medallions (center + 4-fold mirrored satellites)
    centers = [(160.0, 120.0, 46.0, 0.0),
               (62.0, 60.0, 30.0, 1.5), (258.0, 60.0, 30.0, 3.0),
               (62.0, 180.0, 30.0, 4.5), (258.0, 180.0, 30.0, 6.0)]
    for i, (cx, cy, R, ph) in enumerate(centers):
        dx = _xx - cx
        dy = _yy - cy
        r = np.hypot(dx, dy) + 1e-6
        spin = t * 0.004 * (1.0 if i % 2 == 0 else -1.0) + ph
        th = np.arctan2(dy, dx) + spin
        s = (t * 0.0035 + ph * 0.7) % 4.0          # side count morph 3->4->5->6->3
        n1 = 3 + int(s)
        n2 = n1 + 1 if n1 < 6 else 3
        m = s - int(s)
        m = m * m * (3 - 2 * m)                    # smoothstep blend
        rr = (1 - m) * ngon_r(th, n1) + m * ngon_r(th, n2)
        d = r / (R * rr)                           # <1 inside the morphing polygon
        inside = d < 1.0
        hue = d * 0.9 + i * 0.19 + t * 0.0012
        shade = np.clip(1.15 - d, 0, 1) ** 0.7
        col = pal(hue) * shade[..., None]
        edge = np.clip(1.0 - np.abs(d - 1.0) * 12.0, 0, 1)
        col = col + edge[..., None] * np.array([0.9, 0.9, 1.0], np.float32)
        mask = inside | (edge > 0.02)
        img = np.where(mask[..., None], col, img)
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os
    import subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_081.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
