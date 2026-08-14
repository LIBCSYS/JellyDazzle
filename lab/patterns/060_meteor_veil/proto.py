# 060 Meteor Veil — mirrored slow meteors with fading ion trails over twinkling sky
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


_rng = np.random.default_rng(600)
_NS = 300
_sx = _rng.uniform(0, W, _NS)
_sy = _rng.uniform(0, H, _NS)
_sv = _rng.uniform(0.10, 0.30, _NS)
_sp = _rng.uniform(0, TAU, _NS)
_sh = _rng.choice([0.58, 0.62, 0.08, 0.66], _NS)
_NM = 120
_mt = np.cumsum(_rng.integers(34, 62, _NM)).astype(np.float64)
_mx0 = _rng.uniform(10, W * 0.55, _NM)
_my0 = _rng.uniform(-16.0, 40.0, _NM)
_mdx = _rng.uniform(0.85, 1.30, _NM)
_mvy = _rng.uniform(0.45, 0.70, _NM)
_mh = _rng.uniform(0, 1, _NM)
_MG = 0.0035
_MLIFE = 265.0


def render(t):
    tt = float(t) + 160.0
    acc = np.zeros((H, W, 3))
    yy = np.linspace(0.0, 1.0, H)[:, None]
    acc[:, :, 2] += 0.10 - 0.04 * yy
    acc[:, :, 0] += 0.03 + 0.03 * yy
    acc[:, :, 1] += 0.035 * yy  # teal horizon rise
    tw = _sv * (0.7 + 0.3 * np.sin(tt * 0.03 + _sp)) * 1.6
    r, g, b = _hsv(_sh, 0.35, 1.0)
    _splat(acc, _sx, _sy, r, g, b, tw)
    live = np.nonzero(_mt < tt)[0][-13:]
    for k in live:
        age = tt - _mt[k]
        amax = min(age, _MLIFE)
        ns = int(amax / 1.5) + 2
        a = np.linspace(0.0, amax, ns)
        x = _mx0[k] + _mdx[k] * a
        y = _my0[k] + _mvy[k] * a + 0.5 * _MG * a * a
        ago = age - a  # frames since each trail point was laid
        wgt = np.exp(-ago / 170.0) * (0.30 + 0.70 * (a / max(amax, 1.0)))
        r, g, b = _hsv(np.full(ns, _mh[k]), 0.68, 1.0)
        _splat(acc, x, y, r, g, b, wgt * 1.6)
        _splat(acc, (W - 1) - x, y, r, g, b, wgt * 1.6)
        if age < _MLIFE:  # bright bolide heads
            hw = np.array([1.0])
            _splat(acc, x[-1:], y[-1:], hw, hw, hw, hw * 1.3)
            _splat(acc, (W - 1) - x[-1:], y[-1:], hw, hw, hw, hw * 1.3)
    return np.clip(acc * 255.0, 0, 255).astype(np.uint8)


def _write_preview():
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    tmp = "/tmp/dzzle_060.ppm"
    with open(tmp, "wb") as fh:
        fh.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        fh.write(np.ascontiguousarray(img).tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null" % (tmp, here))


if __name__ == "__main__":
    _write_preview()
