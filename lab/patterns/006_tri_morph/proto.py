import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
S3 = np.sqrt(3.0)

PA = np.array([0.52, 0.48, 0.45])
PB = np.array([0.45, 0.45, 0.40])
PC = np.array([1.0, 1.0, 1.0])
PD = np.array([0.85, 0.15, 0.35])


def pal(v):
    rgb = PA + PB * np.cos(2 * np.pi * (PC * v[..., None] + PD))
    return (np.clip(rgb, 0, 1) * 255).astype(np.uint8)


def src(th, r, t):
    """Chiral swirl source — chirality is visible only when mirrors are OFF."""
    return (0.5
            + 0.30 * np.sin(3.0 * th + r * 0.16 - t * 0.010)
            + 0.20 * np.sin(r * 0.09 - t * 0.007 + np.sin(th * 3.0)))


def render(t):
    t = float(t)
    L = 78.0
    sx = _xx - W / 2 + t * 0.08
    sy = _yy - H / 2 + t * 0.04
    u = sx - sy / S3
    v = 2.0 * sy / S3
    du = u - L * np.round(u / L)
    dv = v - L * np.round(v / L)
    dx = du + dv * 0.5
    dy = dv * (S3 / 2)
    r = np.hypot(dx, dy)
    th = np.arctan2(dy, dx)
    # C6 rotation-only fold (chiral, p6-flavored)
    w2 = np.pi / 3
    thA = th % w2
    # D6 mirror fold (p6m snowflake)
    w = np.pi / 6
    thB = np.abs((th % (2 * w)) - w)
    vA = src(thA, r, t)
    vB = src(thB, r, t)
    m = 0.5 + 0.5 * np.sin(t * 0.004)              # mirrors fade in and out
    m = m * m * (3 - 2 * m)
    val = vA * (1 - m) + vB * m
    return pal(val * 0.9 + r * 0.0015 + t * 0.0004)


if __name__ == "__main__":
    import os, subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_006.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
