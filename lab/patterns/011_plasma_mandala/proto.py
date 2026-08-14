"""011 plasma_mandala — classic 4-sine plasma folded 6 ways into a breathing mandala."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_r = np.sqrt(_dx * _dx + _dy * _dy)
_a = np.arctan2(_dy, _dx)


def _fold(ang, n):
    w = 2 * np.pi / n
    fa = np.mod(ang, w)
    return np.where(fa > w * 0.5, w - fa, fa)


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
    n = 6
    fa = _fold(_a + t * 0.004, n)          # whole mandala rotates very slowly
    u = _r * np.cos(fa)
    v = _r * np.sin(fa)
    t1, t2, t3, t4 = t * 0.021, t * 0.017, t * 0.013, t * 0.019
    c = (np.sin(u * 0.046 + t1) + np.sin(v * 0.071 - t2)
         + np.sin((u + v) * 0.038 + t3) + np.sin(_r * 0.052 - t4))
    hue = c * 0.14 + t * 0.0009
    val = 0.46 + 0.36 * np.cos(c * 1.9 - t * 0.011) + 0.16 * np.cos(_r * 0.03 - t * 0.008)
    val = np.clip(val, 0.06, 1.0)
    sat = np.clip(0.78 + 0.22 * np.sin(c * 0.9 + t * 0.006), 0.0, 1.0)
    return (_hsv(hue, sat, val) * 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz011.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
