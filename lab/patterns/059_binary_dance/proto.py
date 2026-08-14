# 059 Binary Dance — gold and cyan suns waltzing, moons and an exchange stream
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


_rng = np.random.default_rng(590)
_NS = 110
_sx = _rng.uniform(0, W, _NS)
_sy = _rng.uniform(0, H, _NS)
_sp = _rng.uniform(0, TAU, _NS)
_OM = 0.0045
_R = 46.0
_HA, _HB = 0.09, 0.55


def _star(times, side):
    a = times * _OM + side
    return CX + _R * np.cos(a), CY + _R * np.sin(a) * 0.78


def render(t):
    tt = float(t) + 400.0
    acc = np.zeros((H, W, 3))
    acc[:, :, 2] += 0.055
    acc[:, :, 0] += 0.025
    tv = 0.08 + 0.06 * np.sin(tt * 0.035 + _sp)
    _splat(acc, _sx, _sy, tv, tv, tv * 1.2, np.ones(_NS))
    # star waltz trails
    j = np.arange(200)
    times = tt - j * 3.2
    fade = (1.0 - j / 200.0) ** 1.4
    for side, hb in ((0.0, _HA), (np.pi, _HB)):
        x, y = _star(times, side)
        sat = 0.85 - 0.6 * np.exp(-j / 5.0)
        r, g, b = _hsv(np.full(200, hb), sat, 1.0)
        _splat(acc, x, y, r, g, b, fade * 1.15)
        # moons: 4 per star with short trails
        jm = np.arange(70)
        tm = tt - jm * 1.0
        fm = (1.0 - jm / 70.0) ** 1.5
        sx, sy = _star(tm, side)
        for m in range(4):
            rm = 13.0 + 4.8 * m
            omm = (0.052 - 0.007 * m) * (1 if m % 2 == 0 else -1)
            ma = omm * tm + m * 1.9 + side
            mx = sx + rm * np.cos(ma)
            my = sy + rm * np.sin(ma) * 0.88
            r, g, b = _hsv(np.full(70, hb + 0.06 * (m - 1.5)), 0.8, 1.0)
            _splat(acc, mx, my, r, g, b, fm * 0.85)
        # star core
        core = np.array([1.0])
        _splat(acc, x[:1], y[:1], core, core, core, core * 1.6)
        ca = np.linspace(0, TAU, 18, endpoint=False)
        for cr in (3.0, 6.0):
            r, g, b = _hsv(np.full(18, hb), 0.5, 1.0)
            _splat(acc, x[0] + cr * np.cos(ca), y[0] + cr * np.sin(ca) * 0.85,
                   r, g, b, np.full(18, 0.55))
    # exchange stream: figure-8 lemniscate rotating with the pair
    ks = np.arange(220)
    s = tt * 0.008 + ks * (TAU / 110.0)
    d = 1.0 + np.sin(s) ** 2
    L = _R * 2.05
    lx = L * np.cos(s) / d
    ly = L * np.sin(s) * np.cos(s) / d * 1.35
    rot = tt * _OM
    cr_, sr_ = np.cos(rot), np.sin(rot)
    x = CX + lx * cr_ - ly * sr_
    y = CY + (lx * sr_ + ly * cr_) * 0.78
    hue = _HA + (_HB - _HA) * (0.5 - 0.5 * np.cos(s))
    r, g, b = _hsv(hue, 0.7, 1.0)
    _splat(acc, x, y, r, g, b, np.full(220, 0.55))
    return np.clip(acc * 255.0, 0, 255).astype(np.uint8)


def _write_preview():
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    tmp = "/tmp/dzzle_059.ppm"
    with open(tmp, "wb") as fh:
        fh.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        fh.write(np.ascontiguousarray(img).tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null" % (tmp, here))


if __name__ == "__main__":
    _write_preview()
