"""015 twister_star — demoscene twister run over radius: a five-armed jewel braid that slowly twists."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_r = np.sqrt(_dx * _dx + _dy * _dy)
_a = np.arctan2(_dy, _dx)


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
    arms = 5
    # twist phase: sine over radius (the "twist") + slow constant spin
    phi = 1.6 * np.sin(_r * 0.028 - t * 0.009) + t * 0.005
    v = _a * arms + phi
    face = np.mod(v, 2 * np.pi)
    fi = np.floor(face / (np.pi / 2)).astype(np.int32) % 4    # 4 faces of the twisted column
    fp = np.mod(face, np.pi / 2) / (np.pi / 2)                # position across the face
    light = np.sin(fp * np.pi) ** 0.7                         # rounded face lighting
    hues = np.array([0.985, 0.09, 0.35, 0.62], np.float32)    # ruby, amber, emerald, sapphire
    hue = hues[fi] + t * 0.0006 + _r * 0.0006
    radial = np.clip(1.0 - _r / 195.0, 0.0, 1.0)
    val = light * (0.22 + 0.78 * radial)
    val = val * np.clip(_r / 12.0, 0.0, 1.0)                  # soften the singular center
    sat = np.full_like(val, 0.85)
    return (_hsv(hue, sat, np.clip(val, 0, 1)) * 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz015.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
