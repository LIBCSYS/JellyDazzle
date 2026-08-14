import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)


def pal(v):
    a = np.array([0.5, 0.5, 0.5], np.float32)
    b = np.array([0.5, 0.5, 0.5], np.float32)
    c = np.array([1.0, 1.0, 1.0], np.float32)
    d = np.array([0.00, 0.33, 0.67], np.float32)
    return np.clip(a + b * np.cos(2 * np.pi * (c * v[..., None] + d)), 0, 1)


def hashn(ix, iy):
    return np.mod(np.sin(ix * 12.9898 + iy * 78.233) * 43758.5453, 1.0)


def render(t):
    t = float(t)
    x = _xx - 160.0
    y = _yy - 120.0
    dman = np.abs(x) + np.abs(y) * 1.45      # wide diamond norm (screen aspect)
    D = 118.0

    # background: near-black with soft horizontal streaks exiting the side vertices
    streak = np.exp(-np.abs(y) / 5.0) * (0.4 + 0.3 * np.sin(x * 0.11 - t * 0.014))
    img = np.zeros((H, W, 3), np.float32)
    img[..., 0] += streak * 0.25
    img[..., 1] += streak * 0.45
    img[..., 2] += streak * 0.55
    img += 0.03

    # concentric diamond rim bands, born at center, expanding outward
    sp = 16.0
    phase = dman / sp - t * 0.010
    ring = np.floor(phase)
    fr = np.mod(phase, 1.0)
    rimhue = np.mod(ring * 0.31 + t * 0.0012, 1.0)   # cyan/magenta/green rotation
    band = np.sin(fr * np.pi) ** 2
    rimzone = (dman < D) & (dman > D * 0.62)
    rimcol = pal(rimhue) * (0.30 + 0.70 * band)[..., None]
    img = np.where(rimzone[..., None], rimcol, img)

    # interior: 4-fold mirrored confetti mosaic, slowly shimmering
    mx = np.abs(x)
    my = np.abs(y)
    ix = np.floor(mx / 8.0)
    iy = np.floor(my / 8.0)
    h = hashn(ix, iy)
    sparkle = 0.5 + 0.5 * np.sin(h * 6.2832 + t * 0.02 + (ix + iy) * 0.4)
    chue = h * 0.85 + t * 0.0015
    inner = dman <= D * 0.62
    innercol = pal(chue) * (0.25 + 0.75 * sparkle)[..., None]
    img = np.where(inner[..., None], innercol, img)

    # bright silhouette edges at the two diamond boundaries
    for edge_d, col in ((D, (0.4, 1.0, 1.0)), (D * 0.62, (1.0, 0.5, 1.0))):
        e = np.clip(1.0 - np.abs(dman - edge_d) / 2.2, 0, 1)
        img = img + e[..., None] * np.array(col, np.float32) * 0.8
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os
    import subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_090.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
