import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
X = _xx - W / 2
Y = _yy - H / 2
R = np.hypot(X, Y)
TH = np.arctan2(Y, X)

PA = np.array([0.50, 0.45, 0.42])
PB = np.array([0.50, 0.42, 0.40])
PC = np.array([1.0, 1.0, 1.0])
PD = np.array([0.00, 0.12, 0.28])


def pal(v):
    rgb = PA + PB * np.cos(2 * np.pi * (PC * v[..., None] + PD))
    return (np.clip(rgb, 0, 1) * 255).astype(np.uint8)


def render(t):
    t = float(t)
    # continuously breathing fold count, 4..16
    N = 10.0 + 6.0 * np.sin(t * 0.0028)
    w = np.pi / N
    th = (TH + t * 0.0022) % (2 * w)
    thf = np.abs(th - w)                           # folded angle in [0, w]
    a = thf / w                                    # normalized wedge coord 0..1
    # radially dominant source hides the single fractional-N seam
    v = (0.5
         + 0.30 * np.sin(R * 0.075 - t * 0.014)
         + 0.16 * np.sin(a * np.pi * 3.0 + R * 0.028 - t * 0.007)
         + 0.10 * np.sin(R * 0.02 + a * np.pi * 6.0 + t * 0.004))
    return pal(v * 0.9 + R * 0.0012 + t * 0.0005)


if __name__ == "__main__":
    import os, subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_005.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
