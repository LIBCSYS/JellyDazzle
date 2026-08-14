import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)


def pal(v):
    a = np.array([0.5, 0.5, 0.5], np.float32)
    b = np.array([0.5, 0.5, 0.5], np.float32)
    c = np.array([1.0, 1.0, 1.0], np.float32)
    d = np.array([0.15, 0.45, 0.75], np.float32)
    return np.clip(a + b * np.cos(2 * np.pi * (c * v[..., None] + d)), 0, 1)


def render(t):
    t = float(t)
    L = 76.0
    x = _xx + t * 0.03            # whole tiling drifts very slowly
    y = _yy + t * 0.018

    # nearest point of the checkerboard lattice (pitch L, offset L/2 sublattice)
    c1x = np.round(x / L) * L
    c1y = np.round(y / L) * L
    c2x = np.round((x - L / 2) / L) * L + L / 2
    c2y = np.round((y - L / 2) / L) * L + L / 2
    d1 = np.hypot(x - c1x, y - c1y)
    d2 = np.hypot(x - c2x, y - c2y)
    use1 = d1 < d2
    cx = np.where(use1, c1x, c2x)
    cy = np.where(use1, c1y, c2y)
    u = x - cx
    v = y - cy
    star_cell = np.mod(np.round(cx / L * 2) + np.round(cy / L * 2), 2) == 0

    cheb = np.maximum(np.abs(u), np.abs(v))
    diam = (np.abs(u) + np.abs(v)) / 1.41421
    d8 = np.minimum(cheb, diam)          # union of 2 squares = 8-point star metric
    cellh = np.mod(np.sin(cx * 0.113 + cy * 0.271) * 43758.5453, 1.0)

    # stars breathe; interior filled with concentric rainbow layers
    Rs = 27.0 + 4.0 * np.sin(t * 0.011 + cellh * 6.28)
    star = star_cell & (d8 < Rs)
    hue_s = d8 / Rs * 0.45 + cellh * 0.35 + t * 0.0013
    bri_s = 0.55 + 0.45 * np.sin(d8 * 0.35 - t * 0.014 + cellh * 6.28)

    # crosses (plus shapes) on the other sublattice, counter-breathing
    wc = 10.0 + 2.5 * np.sin(t * 0.011 + 3.0 + cellh * 6.28)
    cross = (~star_cell) & (np.minimum(np.abs(u), np.abs(v)) < wc) & (cheb < 30.0)
    hue_c = 0.55 + cellh * 0.25 + t * 0.0013
    bri_c = 0.60 + 0.40 * np.sin(cheb * 0.30 + t * 0.012)

    # dark woven ground between objects
    g = 0.06 + 0.05 * (0.5 + 0.5 * np.sin((x + y) * 0.15 - t * 0.006))
    img = g[..., None] * np.array([0.6, 0.5, 1.0], np.float32)
    img = np.where(star[..., None], pal(hue_s) * bri_s[..., None], img)
    img = np.where(cross[..., None], pal(hue_c) * bri_c[..., None], img)

    # thin bright outline on stars
    rim = star_cell & (np.abs(d8 - Rs) < 1.6)
    img = np.where(rim[..., None], np.array([1.0, 0.95, 0.75], np.float32), img)
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os
    import subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_088.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
