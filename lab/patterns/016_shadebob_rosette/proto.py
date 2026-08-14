"""016 shadebob_rosette — Lissajous shadebob trail accumulated in a 6-fold wedge: glowing woven rosettes."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_r = np.sqrt(_dx * _dx + _dy * _dy)
_a = np.arctan2(_dy, _dx)

N = 6
_W2 = 2 * np.pi / N
_fa = np.mod(_a, _W2)
_fa = np.where(_fa > _W2 * 0.5, _W2 - _fa, _fa)
_px = _r * np.cos(_fa)
_py = _r * np.sin(_fa)


def _folds(ang):
    fa = ang % _W2
    return _W2 - fa if fa > _W2 * 0.5 else fa


def _hsv(h, s, v):
    h = np.mod(h, 1.0) * 6.0
    i = np.floor(h).astype(np.int32) % 6
    f = h - np.floor(h)
    p, q, tt = v * (1 - s), v * (1 - s * f), v * (1 - s * (1 - f))
    r = np.choose(i, [v, q, p, p, tt, v])
    g = np.choose(i, [tt, v, v, q, p, p])
    b = np.choose(i, [p, p, tt, v, v, q])
    return np.stack([r, g, b], -1)


def render(t):
    field = np.zeros((H, W), np.float32)
    K = 150                                     # trail length (frames of history)
    for j in range(K):
        tj = t - j * 3.2
        wgt = (1.0 - j / K) ** 1.4              # older trail fades
        bx = 112.0 * np.sin(tj * 0.0123)        # Lissajous bob path
        by = 88.0 * np.sin(tj * 0.0177 + 1.1)
        ba = np.arctan2(by, bx)
        br = np.hypot(bx, by)
        fba = _folds(ba)                         # fold the bob into the wedge
        qx, qy = br * np.cos(fba), br * np.sin(fba)
        d2 = (_px - qx) ** 2 + (_py - qy) ** 2
        field += 0.30 * wgt * np.exp(-d2 / (2 * 7.0 ** 2))
    g = np.tanh(field)                          # soft saturation: crossings glow, no washout
    hue = 0.78 - g * 0.62 + t * 0.0005          # indigo -> magenta -> gold at the hot knots
    val = g ** 0.75
    sat = np.clip(1.05 - g * 0.35, 0.25, 1.0)
    return (_hsv(hue, sat, val) * 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz016.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
