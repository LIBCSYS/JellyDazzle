# 056 Ribbon Swarm — a Lissajous murmuration folding and unfolding, mirrored
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


_N = 300
_u = np.arange(_N) / float(_N)


def render(t):
    tt = float(t) + 150.0
    acc = np.zeros((H, W, 3))
    acc[:, :, 2] += 0.07
    acc[:, :, 0] += 0.03
    spread = 1.7 + 1.1 * np.sin(tt * 0.0019)
    ph = _u * TAU * spread
    hue = _u * 0.9 + tt * 0.0006
    NTR = 9
    for j in range(NTR):
        ttj = tt - j * 2.2
        th = ttj * 0.0095
        drift = ttj * 0.0031
        x0 = 118.0 * np.sin(2 * th + ph)
        y0 = 86.0 * np.sin(3 * th + ph * 1.5 + drift)
        fade = (1.0 - j / float(NTR)) ** 1.4
        sat = 0.92 if j else 0.5
        val = 1.0 - 0.30 * (j / float(NTR))
        r, g, b = _hsv(hue, sat, val)
        _splat(acc, CX + x0, CY + y0, r, g, b, np.full(_N, fade * 0.85))
        _splat(acc, CX - x0, CY + y0, r, g, b, np.full(_N, fade * 0.85))
        _splat(acc, CX + x0, CY - y0, r, g, b, np.full(_N, fade * 0.28))
        _splat(acc, CX - x0, CY - y0, r, g, b, np.full(_N, fade * 0.28))
    return np.clip(acc * 255.0, 0, 255).astype(np.uint8)


def _write_preview():
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    tmp = "/tmp/dzzle_056.ppm"
    with open(tmp, "wb") as fh:
        fh.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        fh.write(np.ascontiguousarray(img).tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null" % (tmp, here))


if __name__ == "__main__":
    _write_preview()
