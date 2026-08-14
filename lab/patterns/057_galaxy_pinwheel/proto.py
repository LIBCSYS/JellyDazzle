# 057 Galaxy Pinwheel — differential-rotation spiral galaxy, gold core to violet rim
import numpy as np, os

W, H = 320, 240
TAU = 2.0 * np.pi
CX, CY = W / 2.0, H / 2.0


def _hsv(h, s, v):
    h, s, v = np.broadcast_arrays(np.asarray(h, np.float64) % 1.0,
                                  np.clip(np.asarray(s, np.float64), 0, 1),
                                  np.clip(np.asarray(v, np.float64), 0, 1))
    h6 = h * 6.0
    i = np.floor(h6).astype(np.int64) % 6
    f = h6 - np.floor(h6)
    p = v * (1 - s); q = v * (1 - s * f); u = v * (1 - s * (1 - f))
    r = np.choose(i, [v, q, p, p, u, v])
    g = np.choose(i, [u, v, v, q, p, p])
    b = np.choose(i, [p, p, u, v, v, q])
    return r, g, b


def _splat(acc, x, y, r, g, b, w):
    x = np.asarray(x, np.float64); y = np.asarray(y, np.float64)
    shp = np.broadcast(x, y).shape
    x = np.broadcast_to(x, shp).ravel(); y = np.broadcast_to(y, shp).ravel()
    full = lambda c: np.broadcast_to(np.asarray(c, np.float64), shp).ravel()
    w = full(w); cols = (full(r) * w, full(g) * w, full(b) * w)
    for dx, dy, k in ((0, 0, 0.85), (1, 0, 0.30), (-1, 0, 0.30), (0, 1, 0.30), (0, -1, 0.30)):
        xi = np.floor(x + dx + 0.5).astype(np.int64)
        yi = np.floor(y + dy + 0.5).astype(np.int64)
        m = (xi >= 0) & (xi < W) & (yi >= 0) & (yi < H)
        if not m.any():
            continue
        for c in range(3):
            np.add.at(acc[:, :, c], (yi[m], xi[m]), cols[c][m] * k)


_rng = np.random.default_rng(570)
_N = 3000
_r = 132.0 * _rng.random(_N) ** 0.72
_arm = _rng.integers(0, 2, _N) * np.pi
_th0 = _arm + _rng.normal(0, 0.30, _N) * (0.35 + _r / 90.0)
_hj = _rng.normal(0, 0.025, _N)
_tw = _rng.uniform(0, TAU, _N)
_spir = 2.5 * np.log1p(_r / 13.0)
_yy, _xx = np.mgrid[0:H, 0:W]
_dist = np.sqrt((_xx - CX) ** 2 + ((_yy - CY) / 0.62) ** 2)
# 9 fixed foreground sparkle stars
_fsx = _rng.uniform(15, W - 15, 9)
_fsy = _rng.uniform(12, H - 12, 9)
_fsp = _rng.uniform(0, TAU, 9)


def render(t):
    tt = float(t) + 400.0
    acc = np.zeros((H, W, 3))
    # deep space + core halo
    acc[:, :, 2] += 0.05 + 0.10 * np.exp(-_dist / 60.0)
    acc[:, :, 0] += 0.015 + 0.16 * np.exp(-(_dist / 26.0) ** 2)
    acc[:, :, 1] += 0.10 * np.exp(-(_dist / 22.0) ** 2)
    th = _th0 + _spir + tt * 0.0042 / (0.45 + _r / 60.0)
    axis = tt * 0.00055
    xr = _r * np.cos(th)
    yr = _r * np.sin(th) * 0.62
    ca, sa = np.cos(axis), np.sin(axis)
    x = CX + xr * ca - yr * sa
    y = CY + (xr * sa + yr * ca) * 0.92
    hue = 0.64 - 0.56 * np.exp(-_r / 55.0) + _hj + tt * 0.00025
    sat = 0.80 - 0.45 * np.exp(-_r / 22.0)
    val = (0.38 + 0.62 * np.exp(-_r / 80.0)) * (0.78 + 0.22 * np.sin(tt * 0.05 + _tw))
    r, g, b = _hsv(hue, sat, 1.0)
    _splat(acc, x, y, r, g, b, val * 1.15)
    # foreground cross-sparkle stars
    fv = 0.35 + 0.30 * np.sin(tt * 0.02 + _fsp)
    for ddx, ddy, kk in ((0, 0, 1.0), (2, 0, 0.4), (-2, 0, 0.4), (0, 2, 0.4), (0, -2, 0.4)):
        _splat(acc, _fsx + ddx, _fsy + ddy, fv, fv, fv * 1.15, np.full(9, kk))
    return np.clip(acc * 255.0, 0, 255).astype(np.uint8)


def _write_preview():
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    tmp = "/tmp/dzzle_057.ppm"
    with open(tmp, "wb") as fh:
        fh.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        fh.write(np.ascontiguousarray(img).tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null" % (tmp, here))


if __name__ == "__main__":
    _write_preview()
