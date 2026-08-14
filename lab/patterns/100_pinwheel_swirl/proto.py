"""100 pinwheel_swirl — faithful replica of dazzle.exe R16 (f-series top-left tile):
smooth-shaded 6-arm comma pinwheel rotating slowly clockwise, spiral tightness
breathing, hue rolling green-dominant -> red-dominant; the one true computed
field (full repaint) among the stamp routines."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_r = np.sqrt(_dx * _dx + _dy * _dy)
_th = np.arctan2(_dy, _dx)
# static ordered-dither noise for the VGA dithered-shading look (never animated)
_rng = np.random.default_rng(77)
_DITH = _rng.uniform(-0.035, 0.035, (H, W)).astype(np.float32)


def _hsv(h, s, v):
    h6 = (h % 1.0) * 6.0
    i = np.floor(h6).astype(np.int32) % 6
    f = h6 - np.floor(h6)
    p, q, tt = v * (1 - s), v * (1 - s * f), v * (1 - s * (1 - f))
    r = np.choose(i, [v, q, p, p, tt, v])
    g = np.choose(i, [tt, v, v, q, p, p])
    b = np.choose(i, [p, p, tt, v, v, q])
    return np.stack([r, g, b], -1) * 255.0


def render(t):
    twist = 0.085 * (1.0 + 0.22 * np.sin(t * 0.003))       # spiral tightness breathes
    v = np.cos(6.0 * _th + twist * _r ** 0.95 - t * 0.012)  # 6 comma arms, cw spin
    shade = np.clip((v + 1.0) * 0.5, 0, 1) ** 1.3
    shade *= np.clip(1.15 - _r / 195.0, 0, 1)               # arms fade at tile edge
    shade = np.clip(shade + _DITH, 0, 1)

    # hue: rim->core gradient along the shading ramp, whole wheel rolling with t
    hue = (0.34 - shade * 0.30 - t * 0.00055) % 1.0   # rolls green -> yellow -> red
    sat = np.clip(1.15 - shade * 0.35, 0, 1)
    val = shade ** 0.85
    return _hsv(hue, sat, val).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz100.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
