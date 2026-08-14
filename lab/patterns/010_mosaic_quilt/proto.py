import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)

PA = np.array([0.50, 0.50, 0.50])
PB = np.array([0.50, 0.50, 0.50])
PC = np.array([1.0, 1.0, 0.5])
PD = np.array([0.80, 0.90, 0.30])


def pal(v):
    rgb = PA + PB * np.cos(2 * np.pi * (PC * v[..., None] + PD))
    return (np.clip(rgb, 0, 1) * 255).astype(np.uint8)


def render(t):
    t = float(t)
    L = 60.0
    sx = _xx + t * 0.10
    sy = _yy + t * 0.06
    gx = np.floor(sx / L)
    gy = np.floor(sy / L)
    u = sx - gx * L - L / 2
    v = sy - gy * L - L / 2
    u = np.abs(u)                                  # 4-fold mirror inside each cell
    v = np.abs(v)
    # alternate cells get the diagonal fold too (p4g flavor checkering)
    par = ((gx + gy) % 2) == 1
    uu = np.where(par, np.maximum(u, v), u)
    vv = np.where(par, np.minimum(u, v), v)
    ph = np.sin(gx * 1.7 + gy * 2.3) * 2.5         # per-cell phase = quilt variety
    d = uu + vv                                    # diamond norm rings per cell
    val = (0.5
           + 0.30 * np.sin(d * 0.22 - t * 0.010 + ph)
           + 0.18 * np.sin((uu - vv) * 0.19 + t * 0.006)
           + 0.10 * np.sin(np.hypot(uu, vv) * 0.24 + ph - t * 0.007))
    hue = val * 0.55 + (gx + gy) * 0.045 + t * 0.0007
    return pal(hue)


if __name__ == "__main__":
    import os, subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_010.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
