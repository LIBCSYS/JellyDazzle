import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
X = _xx - W / 2
Y = _yy - H / 2
TH = np.arctan2(Y, X)
INV_S2 = 1.0 / np.sqrt(2.0)

PA = np.array([0.50, 0.48, 0.52])
PB = np.array([0.48, 0.46, 0.46])
PC = np.array([2.0, 1.0, 1.0])
PD = np.array([0.50, 0.20, 0.25])


def pal(v):
    rgb = PA + PB * np.cos(2 * np.pi * (PC * v[..., None] + PD))
    return (np.clip(rgb, 0, 1) * 255).astype(np.uint8)


def render(t):
    t = float(t)
    rot = t * 0.0016
    c, s = np.cos(rot), np.sin(rot)
    x = X * c - Y * s
    y = X * s + Y * c
    ax, ay = np.abs(x), np.abs(y)
    cheb = np.maximum(ax, ay)                     # square rings (L-inf norm)
    diam = (ax + ay) * INV_S2                     # diamond rings (rotated L1)
    octn = np.maximum(cheb, (ax + ay) * 0.5411 * 1.0824)  # octagon norm
    # polygon norm morphs square -> octagon -> diamond and back
    k = 0.5 + 0.5 * np.sin(t * 0.0024)
    q = cheb * (1 - k) + diam * k
    q = 0.5 * q + 0.5 * octn
    rings = np.sin(q * 0.17 - t * 0.011)
    petals = 0.6 + 0.4 * np.sin(8.0 * TH + 8.0 * rot + 0.35 * q * 0.1)
    v = 0.5 + 0.42 * rings * petals
    return pal(v * 0.9 + q * 0.0015 + t * 0.0004)


if __name__ == "__main__":
    import os, subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_008.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
