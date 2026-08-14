# 051 Firework Garden — accumulating gravity-drooped bursts on a plum ground
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


_rng = np.random.default_rng(510)
_NB = 64
_bt = np.cumsum(_rng.integers(24, 44, _NB)).astype(np.float64)
_bx = _rng.uniform(34, W - 34, _NB)
_by = _rng.uniform(26, H * 0.60, _NB)
_bh = _rng.uniform(0.0, 1.0, _NB)
_NP = 44
_ang = _rng.uniform(0, TAU, (_NB, _NP))
_spd = _rng.uniform(0.5, 1.45, (_NB, _NP)) * (1.0 + 0.15 * np.sin(_ang * 3))
_G = 0.0105
_LIFE = 95.0


def render(t):
    tt = float(t) + 110.0
    acc = np.zeros((H, W, 3))
    yy = np.linspace(0.0, 1.0, H)[:, None]
    acc[:, :, 0] += 0.09 + 0.06 * yy
    acc[:, :, 1] += 0.015
    acc[:, :, 2] += 0.12 + 0.09 * yy
    live = np.nonzero(_bt < tt)[0]
    for k in live[-20:]:
        age = tt - _bt[k]
        amax = min(age, _LIFE)
        ns = int(amax) + 2
        a = np.linspace(0.0, amax, ns)[None, :]
        ca = np.cos(_ang[k])[:, None]; sa = np.sin(_ang[k])[:, None]
        sp = _spd[k][:, None]
        x = _bx[k] + ca * sp * a
        y = _by[k] + sa * sp * a * 0.85 + 0.5 * _G * a * a
        fade = (a / max(amax, 1.0)) ** 1.7          # tail dim -> head bright
        old = np.exp(-max(age - _LIFE, 0.0) / 150.0)  # burnt bursts sink into ground
        wgt = (0.15 + 0.85 * fade) * np.ones((_NP, 1)) * old * 0.9
        hue = (_bh[k] + 0.05 * np.sin(_ang[k]))[:, None] + np.zeros((1, ns))
        r, g, b = _hsv(hue, 0.85, 1.0)
        _splat(acc, x, y, r, g, b, wgt)
        if age < _LIFE:  # hot white heads while burst lives
            one = np.ones(_NP)
            _splat(acc, x[:, -1], y[:, -1], one, one, one, one * 0.8)
    return np.clip(acc * 255.0, 0, 255).astype(np.uint8)


def _write_preview():
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    tmp = "/tmp/dzzle_051.ppm"
    with open(tmp, "wb") as fh:
        fh.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        fh.write(np.ascontiguousarray(img).tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null" % (tmp, here))


if __name__ == "__main__":
    _write_preview()
