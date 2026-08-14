import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
X = _xx - W / 2
Y = _yy - H / 2
R = np.hypot(X, Y)
TH = np.arctan2(Y, X)

PA = np.array([0.5, 0.5, 0.5])
PB = np.array([0.5, 0.5, 0.5])
PC = np.array([1.0, 1.0, 1.0])
PD = np.array([0.00, 0.33, 0.67])


def pal(v):
    rgb = PA + PB * np.cos(2 * np.pi * (PC * v[..., None] + PD))
    return (np.clip(rgb, 0, 1) * 255).astype(np.uint8)


def fold(N, rot):
    """Dihedral D_N mirror fold of the screen angle. Returns folded angle in [0, pi/N]."""
    w = np.pi / N
    th = (TH + rot) % (2 * w)
    return np.abs(th - w)


def field(thf, N, t):
    """Rose-curve interference field sampled in the folded wedge."""
    rose = R - 78.0 * np.abs(np.cos(3.0 * thf * N / 4 + t * 0.006))
    v = (0.50
         + 0.28 * np.sin(R * 0.055 - t * 0.012 + 2.5 * np.cos(thf * 6.0))
         + 0.22 * np.sin(rose * 0.09 + t * 0.004))
    return v


SEQ = [4, 6, 8, 10, 12, 14, 16, 12, 8, 6]
P = 240  # frames per fold stage


def render(t):
    t = float(t)
    i = int(t // P) % len(SEQ)
    j = (i + 1) % len(SEQ)
    f = (t % P) / P
    m = np.clip((f - 0.7) / 0.3, 0.0, 1.0)
    m = m * m * (3 - 2 * m)          # smoothstep crossfade at stage end
    rot = t * 0.0025
    v = field(fold(SEQ[i], rot), SEQ[i], t)
    if m > 0:
        v2 = field(fold(SEQ[j], rot), SEQ[j], t)
        v = v * (1 - m) + v2 * m
    return pal(v * 0.85 + t * 0.0006)


if __name__ == "__main__":
    import os, subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_001.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
