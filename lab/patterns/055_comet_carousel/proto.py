# 055 Comet Carousel — three comets, six-fold kaleidoscope, rotating tail pinwheel
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
_dist = np.sqrt((_xx - CX) ** 2 + (_yy - CY) ** 2)
_NC = 3
_TRAIL = 180


def render(t):
    tt = float(t) + 300.0
    acc = np.zeros((H, W, 3))
    acc[:, :, 2] += 0.055 + 0.06 * np.exp(-_dist / 100.0)
    acc[:, :, 0] += 0.035 * np.exp(-_dist / 70.0)
    grot = tt * 0.0021
    j = np.arange(_TRAIL)
    times = tt - j * 1.5
    fade = (1.0 - j / _TRAIL) ** 1.4
    for c in range(_NC):
        om = 0.016 + 0.0055 * c
        th = times * om + c * 2.1
        rad = 38.0 + 20.0 * c + 30.0 * np.sin(times * 0.0075 + c * 1.7)
        x0 = rad * np.cos(th)
        y0 = rad * np.sin(th) * 0.85
        hue = (c * 0.31 + tt * 0.0006) + 0.11 * (j / _TRAIL)
        sat = 0.85 - 0.6 * np.exp(-j / 6.0)
        val = 1.0 - 0.30 * (j / _TRAIL)
        r, g, b = _hsv(hue, sat, val)
        for m in range(6):
            a = grot + m * TAU / 6.0
            ca, sa = np.cos(a), np.sin(a)
            x = CX + x0 * ca - y0 * sa
            y = CY + (x0 * sa + y0 * ca) * 0.92
            _splat(acc, x, y, r, g, b, fade * 1.05)
    # center medallion: two counter-rotating dot rings
    for ring in range(2):
        rr_ = 7.0 + 6.0 * ring + 2.0 * np.sin(tt * 0.02 + ring * 2.0)
        ns = 12 + 8 * ring
        aa = np.linspace(0, TAU, ns, endpoint=False) + tt * 0.015 * (1 if ring else -1)
        x = CX + rr_ * np.cos(aa)
        y = CY + rr_ * np.sin(aa) * 0.92
        r, g, b = _hsv(np.full(ns, 0.12 + tt * 0.0006 + 0.3 * ring), 0.6, 1.0)
        _splat(acc, x, y, r, g, b, np.full(ns, 0.85))
    return np.clip(acc * 255.0, 0, 255).astype(np.uint8)


def _write_preview():
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    tmp = "/tmp/dzzle_055.ppm"
    with open(tmp, "wb") as fh:
        fh.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        fh.write(np.ascontiguousarray(img).tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null" % (tmp, here))


if __name__ == "__main__":
    _write_preview()
