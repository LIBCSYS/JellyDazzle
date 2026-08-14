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
    seg = 2.0 * np.pi / n
    a = np.mod(theta, seg) - seg / 2
    return np.cos(np.pi / n) / np.cos(a)


def render(t):
    t = float(t)
    # deep-blue field with faint breathing rings
    dx = _xx - 160.0
    dy = _yy - 120.0
    r0 = np.hypot(dx, dy)
    f = 0.5 + 0.5 * np.sin(r0 * 0.09 - t * 0.007)
    img = f[..., None] * np.array([0.05, 0.07, 0.16], np.float32)
    img = img + np.array([0.02, 0.02, 0.08], np.float32)

    # ring of 8 faceted hexagonal gems orbiting a large center gem
    orb = t * 0.0025
    gems = [(160.0 + 96.0 * np.cos(orb + k * np.pi / 4),
             120.0 + 72.0 * np.sin(orb + k * np.pi / 4), k) for k in range(8)]
    gems.append((160.0, 120.0, 8))
    for (cx, cy, k) in gems:
        R = 17.0 if k < 8 else 27.0
        dx = _xx - cx
        dy = _yy - cy
        r = np.hypot(dx, dy) + 1e-6
        th = np.arctan2(dy, dx) + t * 0.008 * (1.0 if k % 2 else -1.0) + k
        d = r / (R * ngon_r(th, 6))
        inside = d < 1.0
        facet = np.floor(np.mod(th / (2.0 * np.pi) * 6.0, 6.0))
        bri = 0.55 + 0.35 * np.sin(facet * 2.1 + t * 0.010 + k * 1.7)
        hue = k * 0.117 + facet * 0.045 + t * 0.0015
        col = pal(hue) * (bri * np.clip(1.3 - d, 0, 1)) [..., None]
        edge = np.clip(1.0 - np.abs(d - 1.0) * 10.0, 0, 1)
        col = col + edge[..., None] * 0.85
        img = np.where((inside | (edge > 0.03))[..., None], col, img)
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os
    import subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_086.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
