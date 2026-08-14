# 054 Starfield Warp — hue-ringed hyperspace drift with wandering vanishing point
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


_rng = np.random.default_rng(540)
_N = 750
_ux = _rng.normal(0, 0.50, _N)
_uy = _rng.normal(0, 0.38, _N)
_z0 = _rng.uniform(0, 1.1, _N)
_tw = _rng.uniform(0, TAU, _N)
_ZMAX = 1.1
_V = 0.0022
_F = 170.0
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float64)


def render(t):
    tt = float(t) + 500.0
    acc = np.zeros((H, W, 3))
    # slow-breathing nebula ground
    n1 = np.exp(-(((_xx - 90 - 30 * np.sin(tt * 0.002)) / 130.0) ** 2 +
                  ((_yy - 70) / 95.0) ** 2))
    n2 = np.exp(-(((_xx - 240 + 25 * np.sin(tt * 0.0016 + 2)) / 120.0) ** 2 +
                  ((_yy - 180) / 100.0) ** 2))
    acc[:, :, 0] += 0.10 * n1 + 0.02 * n2
    acc[:, :, 1] += 0.02 * n1 + 0.06 * n2
    acc[:, :, 2] += 0.16 * n1 + 0.13 * n2 + 0.03
    cx = CX + 16.0 * np.sin(tt * 0.0045)
    cy = CY + 11.0 * np.sin(tt * 0.0031 + 1.2)
    z = ((_z0 - _V * tt) % _ZMAX) + 0.07
    zp = z + _V * 9.0  # position ~9 frames back -> streak
    hue = (np.arctan2(_uy, _ux) / TAU + tt * 0.0005 + _z0 * 0.12)
    NS = 10
    for si in range(NS):
        f = si / (NS - 1.0)
        zi = z * (1 - f) + zp * f
        px = cx + _ux * _F / zi
        py = cy + _uy * _F / zi
        bright = np.clip(0.34 / zi, 0.05, 1.3) * (1.0 - 0.70 * f)
        bright *= 0.8 + 0.2 * np.sin(tt * 0.05 + _tw)
        sat = np.clip(zi * 1.15, 0.15, 0.8)
        r, g, b = _hsv(hue, sat, 1.0)
        _splat(acc, px, py, r, g, b, bright)
    return np.clip(acc * 255.0, 0, 255).astype(np.uint8)


def _write_preview():
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    tmp = "/tmp/dzzle_054.ppm"
    with open(tmp, "wb") as fh:
        fh.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        fh.write(np.ascontiguousarray(img).tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null" % (tmp, here))


if __name__ == "__main__":
    _write_preview()
