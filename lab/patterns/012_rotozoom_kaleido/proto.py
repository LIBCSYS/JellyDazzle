"""012 rotozoom_kaleido — rotozoomed procedural tile texture inside an 8-fold mirror; sunset palette."""
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


def _lut(stops):
    lut = np.zeros((256, 3), np.float32)
    for (p0, c0), (p1, c1) in zip(stops[:-1], stops[1:]):
        i0, i1 = int(p0 * 255), int(p1 * 255)
        for ch in range(3):
            lut[i0:i1 + 1, ch] = np.linspace(c0[ch], c1[ch], i1 - i0 + 1)
    return lut


# looping sunset palette: dusk purple -> magenta -> ember red -> gold -> cream -> back
_PAL = _lut([
    (0.00, (24, 6, 48)),
    (0.20, (110, 16, 130)),
    (0.42, (232, 58, 72)),
    (0.62, (255, 170, 52)),
    (0.80, (255, 238, 196)),
    (1.00, (24, 6, 48)),
])


def render(t):
    n = 8
    fa = _fold(_a, n)
    px, py = _r * np.cos(fa), _r * np.sin(fa)
    th = t * 0.006                                 # slow spin
    k = 0.085 * (1.0 + 0.42 * np.sin(t * 0.0045))  # zoom pulse (mandala inhales/exhales)
    co, si = np.cos(th), np.sin(th)
    u = (px * co - py * si) * k + t * 0.010        # slow drift across the texture
    v = (px * si + py * co) * k + t * 0.007
    f = np.sin(u * 2.2) * np.sin(v * 2.2) + 0.6 * np.sin(u + v) + 0.35 * np.sin(u * 0.7 - v * 0.9)
    idx = np.clip((f + 1.95) / 3.9 * 255.0, 0, 255).astype(np.int32)
    return _PAL[idx].astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz012.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
