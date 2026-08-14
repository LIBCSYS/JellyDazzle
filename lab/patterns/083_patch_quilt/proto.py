import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)


def pal(v):
    a = np.array([0.5, 0.5, 0.5], np.float32)
    b = np.array([0.5, 0.5, 0.5], np.float32)
    c = np.array([1.0, 1.0, 1.0], np.float32)
    d = np.array([0.10, 0.42, 0.75], np.float32)
    return np.clip(a + b * np.cos(2 * np.pi * (c * v[..., None] + d)), 0, 1)


def hash2(gx, gy):
    return np.mod(np.sin(gx * 127.1 + gy * 311.7) * 43758.5453, 1.0)


def render(t):
    t = float(t)
    L = 48.0
    x = _xx + 8.0   # offset so seams don't hug the border
    y = _yy + 8.0
    gx = np.floor(x / L)
    gy = np.floor(y / L)
    u = x - gx * L - L / 2
    v = y - gy * L - L / 2
    h = hash2(gx, gy)
    mid = np.floor(h * 4)            # which motif lives in this patch
    ph = h * 6.2832

    r = np.hypot(u, v)
    dman = np.abs(u) + np.abs(v)
    f0 = 0.5 + 0.5 * np.sin(dman * 0.30 - t * 0.016 + ph)            # nested diamonds
    f1 = 0.5 + 0.5 * np.sin(r * 0.35 + t * 0.014 + ph)               # target rings
    f2 = 0.5 + 0.5 * np.sin(np.minimum(np.abs(u), np.abs(v)) * 0.55
                            - t * 0.012 + ph)                        # plus/cross bands
    f3 = np.sin(u * 0.45 + t * 0.010 + ph) * np.sin(v * 0.45 - t * 0.010)
    f3 = f3 * 0.5 + 0.5                                              # woven checker
    fld = np.select([mid == 0, mid == 1, mid == 2], [f0, f1, f2], f3)

    hue = h * 0.9 + fld * 0.22 + t * 0.0012
    img = pal(hue) * (0.30 + 0.70 * fld)[..., None]

    # sashing (stitch lines between patches) with corner buttons
    b = np.minimum(np.minimum(u + L / 2, L / 2 - u),
                   np.minimum(v + L / 2, L / 2 - v))
    sash = b < 2.5
    sashcol = np.array([0.10, 0.07, 0.18], np.float32) + 0.05 * np.sin(t * 0.01)
    img = np.where(sash[..., None], sashcol[None, None, :] * np.ones_like(img), img)
    button = (np.abs(np.abs(u) - L / 2) < 5) & (np.abs(np.abs(v) - L / 2) < 5)
    bcol = pal(np.full_like(u, 0.12 + t * 0.0012)) * (0.9 + 0.1 * np.sin(t * 0.02 + ph))[..., None]
    img = np.where(button[..., None], bcol, img)
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os
    import subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_083.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
