"""017 metaball_kiss — metaballs in a 6-fold wedge that merge with their own reflections across the seams."""
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


# (wx, wy, phase_x, phase_y, amp_x, amp_y, strength)
_BALLS = [
    (0.0071, 0.0053, 0.0, 1.2, 105, 82, 2400),
    (0.0049, 0.0067, 2.4, 0.4, 88, 96, 2000),
    (0.0061, 0.0043, 4.1, 2.8, 116, 70, 2100),
    (0.0037, 0.0079, 1.7, 4.6, 72, 104, 1700),
]


def render(t):
    g = np.zeros((H, W), np.float32)
    for wx, wy, p1, p2, ax, ay, s in _BALLS:
        bx = ax * np.sin(t * wx + p1)
        by = ay * np.sin(t * wy + p2)
        ba = np.arctan2(by, bx)
        br = np.hypot(bx, by)
        fba = _folds(ba)                       # fold ball into the wedge
        qx, qy = br * np.cos(fba), br * np.sin(fba)
        d2 = (_px - qx) ** 2 + (_py - qy) ** 2
        g += s / (d2 + 160.0)
    inside = np.tanh((g - 1.0) * 2.2) * 0.5 + 0.5          # soft iso threshold
    rim = np.exp(-((g - 1.0) ** 2) / 0.030)                # glowing merge contour
    bands = 0.72 + 0.28 * np.sin(np.log(g + 0.2) * 5.0 - t * 0.018)  # broad slow inner contours
    hue = 0.50 - inside * 0.17 + t * 0.0006                # cyan skin -> emerald core
    body = np.clip((g - 0.55) * 1.1, 0, 1) ** 1.2          # dark ground until a blob is near
    val = np.clip(body * bands + rim * 0.55, 0, 1)
    sat = np.clip(0.9 - rim * 0.55, 0.0, 1.0)              # rim burns toward white
    rgb = _hsv(hue, sat, val)
    rgb[..., 0] += rim * 0.35                              # magenta kiss on the rim
    rgb[..., 2] += rim * 0.25
    rgb[..., 2] += (1 - np.clip(body * 3, 0, 1)) * 0.05    # faint abyss-blue floor
    return (np.clip(rgb, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz017.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
