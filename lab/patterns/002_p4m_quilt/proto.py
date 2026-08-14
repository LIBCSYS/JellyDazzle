import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)

PA = np.array([0.50, 0.46, 0.50])
PB = np.array([0.48, 0.44, 0.42])
PC = np.array([1.0, 1.0, 1.0])
PD = np.array([0.00, 0.15, 0.42])


def pal(v):
    rgb = PA + PB * np.cos(2 * np.pi * (PC * v[..., None] + PD))
    return (np.clip(rgb, 0, 1) * 255).astype(np.uint8)


def render(t):
    t = float(t)
    L = 52.0 + 18.0 * np.sin(t * 0.004)          # cell size breathes
    sx = _xx + t * 0.16                           # slow lattice drift
    sy = _yy + t * 0.09
    # p4m fold: triangle-wave mirror both axes, then diagonal fold
    u = sx % (2 * L)
    u = np.where(u >= L, 2 * L - u, u)
    v = sy % (2 * L)
    v = np.where(v >= L, 2 * L - v, v)
    uu = np.maximum(u, v)
    vv = np.minimum(u, v)
    # plasma source evaluated in the fundamental 45-45-90 triangle
    d = np.hypot(uu - L * 0.5, vv - L * 0.5)
    val = (np.sin(uu * 0.11 + t * 0.008)
           + np.sin(vv * 0.13 - t * 0.006)
           + np.sin((uu + vv) * 0.065 + t * 0.005)
           + np.sin(d * 0.17 - t * 0.010))
    val = val * 0.125 + 0.5
    return pal(val + t * 0.0005)


if __name__ == "__main__":
    import os, subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_002.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
