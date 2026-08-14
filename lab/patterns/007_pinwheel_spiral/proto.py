import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
X = _xx - W / 2
Y = _yy - H / 2
R = np.hypot(X, Y)
TH = np.arctan2(Y, X)
LNR = np.log(R + 4.0)

PA = np.array([0.50, 0.50, 0.50])
PB = np.array([0.50, 0.50, 0.50])
PC = np.array([1.0, 0.7, 0.4])
PD = np.array([0.00, 0.15, 0.20])


def pal(v):
    rgb = PA + PB * np.cos(2 * np.pi * (PC * v[..., None] + PD))
    return (np.clip(rgb, 0, 1) * 255).astype(np.uint8)


def arms(N, t):
    """C_N rotational (no mirror) log-spiral pinwheel field."""
    a = N * TH + 3.4 * np.sin(t * 0.0015) * LNR - t * 0.016
    return (0.5
            + 0.32 * np.sin(a)
            + 0.16 * np.sin(2.0 * a + R * 0.03 + t * 0.005))


SEQ = [6, 9, 12, 8, 10]
P = 300


def render(t):
    t = float(t)
    i = int(t // P) % len(SEQ)
    j = (i + 1) % len(SEQ)
    f = (t % P) / P
    m = np.clip((f - 0.72) / 0.28, 0.0, 1.0)
    m = m * m * (3 - 2 * m)
    v = arms(SEQ[i], t)
    if m > 0:
        v = v * (1 - m) + arms(SEQ[j], t) * m
    return pal(v * 0.85 + R * 0.0018 + t * 0.0005)


if __name__ == "__main__":
    import os, subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_007.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
