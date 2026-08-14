import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
RSC = np.hypot(_xx - W / 2, _yy - H / 2)

PA = np.array([0.52, 0.50, 0.48])
PB = np.array([0.48, 0.48, 0.46])
PC = np.array([1.0, 1.0, 1.0])
PD = np.array([0.10, 0.40, 0.70])


def cpal(v):
    return np.clip(PA + PB * np.cos(2 * np.pi * (PC * v[..., None] + PD)), 0, 1)


SEQ = [5, 7, 9, 12, 8, 6]
P = 320


def render(t):
    t = float(t)
    N = SEQ[int(t // P) % len(SEQ)]
    # lissajous curve segment (trailing window), detuned for slow precession
    n = 4200
    s = np.arange(n, dtype=np.float64) * 0.006 + t * 0.012
    px = 92.0 * np.sin(3.0 * s + t * 0.0015)
    py = 92.0 * np.sin(4.0 * s * 1.0012)
    wgt = np.linspace(0.15, 1.0, n)               # tail fades out
    # stamp N rotated copies about center (C_N symmetry)
    ang = np.arange(N) * (2 * np.pi / N) + t * 0.0018
    ca, sa = np.cos(ang), np.sin(ang)
    gx = (px[None, :] * ca[:, None] - py[None, :] * sa[:, None]).ravel() + W / 2
    gy = (px[None, :] * sa[:, None] + py[None, :] * ca[:, None]).ravel() + H / 2
    gw = np.tile(wgt, N)
    buf = np.zeros((H, W), dtype=np.float64)
    ix = np.floor(gx).astype(np.int64)
    iy = np.floor(gy).astype(np.int64)
    fx = gx - ix
    fy = gy - iy
    ok = (ix >= 0) & (ix < W - 1) & (iy >= 0) & (iy < H - 1)
    ix, iy, fx, fy, gw = ix[ok], iy[ok], fx[ok], fy[ok], gw[ok]
    np.add.at(buf, (iy, ix), gw * (1 - fx) * (1 - fy))
    np.add.at(buf, (iy, ix + 1), gw * fx * (1 - fy))
    np.add.at(buf, (iy + 1, ix), gw * (1 - fx) * fy)
    np.add.at(buf, (iy + 1, ix + 1), gw * fx * fy)
    # soften once (cheap 3x3 box via rolls)
    b = buf.copy()
    for dx0, dy0 in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        b += np.roll(np.roll(buf, dy0, axis=0), dx0, axis=1)
    glow = 1.0 - np.exp(-b * 0.55)
    col = cpal(glow * 0.55 + RSC * 0.0022 + t * 0.0006)
    ground = cpal(RSC * 0.001 + t * 0.0004) * 0.10
    out = ground * (1 - glow[..., None]) + col * glow[..., None]
    return (np.clip(out, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_009.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
