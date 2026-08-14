import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
S3 = np.sqrt(3.0)

PA = np.array([0.48, 0.50, 0.52])
PB = np.array([0.42, 0.46, 0.48])
PC = np.array([1.0, 1.0, 1.0])
PD = np.array([0.58, 0.83, 0.05])


def pal(v):
    rgb = PA + PB * np.cos(2 * np.pi * (PC * v[..., None] + PD))
    return (np.clip(rgb, 0, 1) * 255).astype(np.uint8)


def render(t):
    t = float(t)
    L = 72.0
    sx = _xx - W / 2 + t * 0.11
    sy = _yy - H / 2 + t * 0.05
    # skewed lattice coords (hex basis inverse)
    u = sx - sy / S3
    v = 2.0 * sy / S3
    # delta to nearest lattice corner (a 6-fold rotocenter of p6m)
    du = u - L * np.round(u / L)
    dv = v - L * np.round(v / L)
    dx = du + dv * 0.5
    dy = dv * (S3 / 2)
    # D6 mirror fold about the rotocenter
    r = np.hypot(dx, dy)
    th = np.arctan2(dy, dx) + t * 0.0018          # slow local spin
    w = np.pi / 6
    thf = np.abs((th % (2 * w)) - w)
    # snowflake source: petal rings + branch stripes
    val = (0.5
           + 0.30 * np.sin(r * 0.16 - t * 0.010 + 4.0 * thf)
           + 0.20 * np.sin(r * 0.05 * np.cos(thf * 6.0) * 6.0 + t * 0.006))
    return pal(val * 0.9 + r * 0.002 + t * 0.0004)


if __name__ == "__main__":
    import os, subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_003.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
