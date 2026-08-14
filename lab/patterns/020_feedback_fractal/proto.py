"""020 feedback_fractal — fold-aware video-feedback zoom: stamped glow blobs + rings recursively re-mirrored into the center, 7-fold."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_r = np.sqrt(_dx * _dx + _dy * _dy)
_a = np.arctan2(_dy, _dx)

N = 7
_W2 = 2 * np.pi / N
_fa = np.mod(_a, _W2)
_fa = np.where(_fa > _W2 * 0.5, _W2 - _fa, _fa)
_PX = _r * np.cos(_fa)
_PY = _r * np.sin(_fa)


def _folds(ang):
    fa = ang % _W2
    return _W2 - fa if fa > _W2 * 0.5 else fa


def _hsv1(h, s, v):
    """scalar hsv -> rgb tuple"""
    h = (h % 1.0) * 6.0
    i = int(h) % 6
    f = h - int(h)
    p, q, tt = v * (1 - s), v * (1 - s * f), v * (1 - s * (1 - f))
    return [(v, tt, p), (q, v, p), (p, v, tt), (p, q, v), (tt, p, v), (v, p, q)][i]


def render(t):
    col = np.zeros((H, W, 3), np.float32)
    rot = 0.42 + 0.10 * np.sin(t * 0.0030)     # per-generation twist drifts slowly
    sc = 1.26                                  # per-generation zoom (deeper copies shrink to center)
    for g in range(-3, 12):                    # negative g = copies larger than the screen (fills corners)
        tg = t - g * 22.0                      # deeper copies lag in time
        decay = 0.84 ** abs(g) * (0.9 if g < 0 else 1.0)
        s = sc ** g
        ang = rot * g
        co, si = np.cos(ang), np.sin(ang)
        x = (_PX * co - _PY * si) * s
        y = (_PX * si + _PY * co) * s
        # stamp 1: orbiting glow blob (folded into the wedge)
        sx = 74 * np.sin(tg * 0.0090 + 1.0)
        sy = 58 * np.sin(tg * 0.0063 + 2.6)
        ba = np.arctan2(sy, sx)
        br = np.hypot(sx, sy)
        fb = _folds(ba)
        qx, qy = br * np.cos(fb), br * np.sin(fb)
        d2 = (x - qx) ** 2 + (y - qy) ** 2
        blob = np.exp(-d2 / (2 * 13.0 ** 2))
        # stamp 2: breathing thin ring
        ring = np.exp(-((np.hypot(x, y) - 82 - 26 * np.sin(tg * 0.005)) ** 2) / (2 * 7.0 ** 2))
        h1 = 0.55 + g * 0.075 + t * 0.0007     # hue shifts with recursion depth
        c1 = np.array(_hsv1(h1, 0.92, 1.0), np.float32)
        c2 = np.array(_hsv1(h1 + 0.45, 0.80, 1.0), np.float32)
        col += decay * (blob[..., None] * c1 + 0.60 * ring[..., None] * c2)
    return (np.clip(col * 0.92, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz020.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
