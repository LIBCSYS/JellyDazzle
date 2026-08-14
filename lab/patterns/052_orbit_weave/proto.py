# 052 Orbit Weave — precessing electron ellipses tracing a woven rosette (c06 homage)
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


_rng = np.random.default_rng(520)
_NS = 130
_sx = _rng.uniform(0, W, _NS)
_sy = _rng.uniform(0, H, _NS)
_sp = _rng.uniform(0, TAU, _NS)
_NORB = 7
_M = 230


def render(t):
    tt = float(t) + 60.0
    acc = np.zeros((H, W, 3))
    acc[:, :, 2] += 0.05
    acc[:, :, 0] += 0.015
    # dust stars twinkling faintly
    tv = 0.10 + 0.08 * np.sin(tt * 0.04 + _sp)
    _splat(acc, _sx, _sy, tv, tv, tv * 1.3, np.ones(_NS))
    for i in range(_NORB):
        prec = 0.0016 + 0.00045 * i
        th = i * (TAU / (2 * _NORB)) + tt * prec
        A = 128.0 - 9.0 * i
        B = 26.0 + 11.0 * i
        om = 0.020 + 0.004 * i
        ph = tt * om + i * 1.3
        s = ph - np.linspace(0.0, 2.6, _M)
        ex = A * np.cos(s); ey = B * np.sin(s)
        c, sn = np.cos(th), np.sin(th)
        x = CX + ex * c - ey * sn
        y = CY + (ex * sn + ey * c) * 0.92
        fade = np.linspace(1.0, 0.05, _M) ** 1.5
        hue = (0.52 + 0.085 * i + tt * 0.0007)
        sat = 0.85 - 0.55 * np.exp(-np.linspace(0, _M, _M) / 7.0)  # white-hot head
        r, g, b = _hsv(np.full(_M, hue), sat, 1.0)
        _splat(acc, x, y, r, g, b, fade * 0.9)
        _splat(acc, 2 * CX - x, 2 * CY - y, r, g, b, fade * 0.9)  # opposite twin
        # faint dotted full orbit ring
        sf = np.linspace(0, TAU, 110, endpoint=False) + tt * 0.002
        fx = CX + (A * np.cos(sf)) * c - (B * np.sin(sf)) * sn
        fy = CY + ((A * np.cos(sf)) * sn + (B * np.sin(sf)) * c) * 0.92
        rr, gg, bb = _hsv(np.full(110, hue), 0.9, 1.0)
        _splat(acc, fx, fy, rr, gg, bb, np.full(110, 0.085))
    # pulsing nucleus of dot rings
    for ring in range(3):
        rr_ = 4.0 + 3.6 * ring + 1.6 * np.sin(tt * 0.03 + ring * 1.1)
        ns = 10 + 6 * ring
        aa = np.linspace(0, TAU, ns, endpoint=False) + tt * 0.012 * (1 if ring % 2 else -1)
        x = CX + rr_ * np.cos(aa); y = CY + rr_ * np.sin(aa)
        r, g, b = _hsv(np.full(ns, (0.11 + tt * 0.0007)), 0.55, 1.0)
        _splat(acc, x, y, r, g, b, np.full(ns, 0.9))
    return np.clip(acc * 255.0, 0, 255).astype(np.uint8)


def _write_preview():
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    tmp = "/tmp/dzzle_052.ppm"
    with open(tmp, "wb") as fh:
        fh.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        fh.write(np.ascontiguousarray(img).tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null" % (tmp, here))


if __name__ == "__main__":
    _write_preview()
