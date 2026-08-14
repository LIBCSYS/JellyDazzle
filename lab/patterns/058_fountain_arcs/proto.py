# 058 Fountain Arcs — sweeping rainbow sprinkler, gravity arcs accumulating, mirrored
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


_rng = np.random.default_rng(580)
_NL = 1200
_tj = np.arange(_NL) * 3.0
_angj = np.pi / 2.0 + 0.72 * np.sin(np.arange(_NL) * 0.045) + _rng.normal(0, 0.03, _NL)
_spdj = 2.45 + 0.30 * np.sin(np.arange(_NL) * 0.11) + _rng.normal(0, 0.08, _NL)
_huej = (np.arange(_NL) * 0.011) % 1.0
_G = 0.030
_BY = H - 14.0


def render(t):
    tt = float(t) + 240.0
    acc = np.zeros((H, W, 3))
    yy = np.linspace(0.0, 1.0, H)[:, None]
    acc[:, :, 2] += 0.05 + 0.07 * yy
    acc[:, :, 1] += 0.02 * yy
    live = np.nonzero(_tj < tt)[0][-120:]
    for k in live:
        age = tt - _tj[k]
        vy = _spdj[k] * np.sin(_angj[k])
        vx = _spdj[k] * np.cos(_angj[k]) * 0.62
        aland = 2.0 * vy / _G + 8.0
        amax = min(age, aland)
        ns = int(amax / 2.0) + 2
        a = np.linspace(0.0, amax, ns)
        x = CX + vx * a
        y = _BY - vy * a + 0.5 * _G * a * a
        oldf = np.exp(-max(age - aland, 0.0) / 260.0)
        wgt = (0.20 + 0.80 * (a / max(amax, 1.0)) ** 1.3) * oldf * 0.6
        r, g, b = _hsv(np.full(ns, _huej[k] + tt * 0.0002), 0.85, 1.0)
        _splat(acc, x, y, r, g, b, wgt)
        _splat(acc, 2 * CX - x, y, r, g, b, wgt)
        if age < aland:  # bright droplet heads
            hw = np.array([0.9])
            _splat(acc, x[-1:], y[-1:], hw, hw, hw, hw)
            _splat(acc, 2 * CX - x[-1:], y[-1:], hw, hw, hw, hw)
    # emitter glow, warm and pulsing
    ga = np.linspace(0, TAU, 26, endpoint=False)
    gr = 5.0 + 1.5 * np.sin(tt * 0.05)
    r, g, b = _hsv(np.full(26, 0.10), 0.4, 1.0)
    _splat(acc, CX + gr * np.cos(ga), _BY + gr * np.sin(ga) * 0.5, r, g, b, np.full(26, 0.7))
    return np.clip(acc * 255.0, 0, 255).astype(np.uint8)


def _write_preview():
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    tmp = "/tmp/dzzle_058.ppm"
    with open(tmp, "wb") as fh:
        fh.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        fh.write(np.ascontiguousarray(img).tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null" % (tmp, here))


if __name__ == "__main__":
    _write_preview()
