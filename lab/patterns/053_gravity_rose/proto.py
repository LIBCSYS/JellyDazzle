# 053 Gravity Rose — precessing Kepler petals, 4-fold mirrored comet trails
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


_yy, _xx = np.mgrid[0:H, 0:W]
_dist = np.sqrt((_xx - CX) ** 2 + ((_yy - CY) * 1.15) ** 2)
_NPART = 8
_TRAIL = 180


def render(t):
    tt = float(t) + 260.0
    acc = np.zeros((H, W, 3))
    # dark navy ground with a warm core glow
    acc[:, :, 2] += 0.06 + 0.05 * np.exp(-_dist / 90.0)
    acc[:, :, 0] += 0.02 + 0.26 * np.exp(-(_dist / 13.0) ** 2)
    acc[:, :, 1] += 0.18 * np.exp(-(_dist / 11.0) ** 2)
    j = np.arange(_TRAIL)
    times = tt - j * 1.35
    fade = (1.0 - j / _TRAIL) ** 1.5
    for p in range(_NPART):
        om = 0.011 + 0.0016 * p
        prec = 0.0021 * (1.0 + 0.35 * p)
        e = 0.38
        phi = om * times + p * 2.399
        rel = phi - prec * times
        rr = (40.0 + 6.5 * p) * (1 - e * e) / (1 + e * np.cos(rel))
        x = rr * np.cos(phi) * 0.98
        y = rr * np.sin(phi) * 0.80
        hue = (p / _NPART + tt * 0.00045)
        sat = 0.85 - 0.55 * np.exp(-j / 6.0)
        val = 1.0 - 0.35 * (j / _TRAIL)
        r, g, b = _hsv(np.full(_TRAIL, hue), sat, val)
        for mx, my in ((1, 1), (-1, 1), (1, -1), (-1, -1)):
            _splat(acc, CX + mx * x, CY + my * y, r, g, b, fade * 0.95)
    return np.clip(acc * 255.0, 0, 255).astype(np.uint8)


def _write_preview():
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    tmp = "/tmp/dzzle_053.ppm"
    with open(tmp, "wb") as fh:
        fh.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        fh.write(np.ascontiguousarray(img).tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null" % (tmp, here))


if __name__ == "__main__":
    _write_preview()
