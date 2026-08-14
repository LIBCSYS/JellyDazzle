"""019 moire_silk — two orbiting interference-ring sources in an 8-fold wedge; teal/ember silk with palette rotation."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_r = np.sqrt(_dx * _dx + _dy * _dy)
_a = np.arctan2(_dy, _dx)

N = 8
_W2 = 2 * np.pi / N
_fa = np.mod(_a, _W2)
_fa = np.where(_fa > _W2 * 0.5, _W2 - _fa, _fa)
_px = _r * np.cos(_fa)
_py = _r * np.sin(_fa)


def _folds(ang):
    fa = ang % _W2
    return _W2 - fa if fa > _W2 * 0.5 else fa


def _lut(stops):
    lut = np.zeros((256, 3), np.float32)
    for (p0, c0), (p1, c1) in zip(stops[:-1], stops[1:]):
        i0, i1 = int(p0 * 255), int(p1 * 255)
        for ch in range(3):
            lut[i0:i1 + 1, ch] = np.linspace(c0[ch], c1[ch], i1 - i0 + 1)
    return lut


# looping silk palette: deep teal -> cyan -> cream -> ember orange -> wine -> teal
_PAL = _lut([
    (0.00, (6, 42, 52)),
    (0.22, (44, 202, 190)),
    (0.45, (250, 242, 212)),
    (0.68, (240, 132, 52)),
    (0.86, (96, 22, 44)),
    (1.00, (6, 42, 52)),
])


def _foldpt(x, y):
    ang = np.arctan2(y, x)
    rr = np.hypot(x, y)
    f = _folds(ang)
    return rr * np.cos(f), rr * np.sin(f)


def render(t):
    c1x, c1y = _foldpt(72 * np.sin(t * 0.0060), 56 * np.sin(t * 0.0043 + 0.8))
    c2x, c2y = _foldpt(66 * np.sin(t * 0.0051 + 2.1), 62 * np.sin(t * 0.0069 + 4.0))
    d1 = np.hypot(_px - c1x, _py - c1y)
    d2 = np.hypot(_px - c2x, _py - c2y)
    s = np.sin(d1 * 0.30 - t * 0.020) + np.sin(d2 * 0.26 + t * 0.016)
    idx = ((s + 2.0) / 4.0 * 255.0 + t * 0.40) % 256.0     # palette rotation = free animation
    return _PAL[idx.astype(np.int32)].astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz019.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
