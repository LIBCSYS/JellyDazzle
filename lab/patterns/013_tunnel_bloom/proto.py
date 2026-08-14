"""013 tunnel_bloom — off-center flying tunnel seen through a 4-fold mirror; ice palette."""
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
    n = 4
    fa = _fold(_a, n)
    px, py = _r * np.cos(fa), _r * np.sin(fa)
    ox = 55 * np.sin(t * 0.005)                 # tunnel mouth orbits off the fold axis
    oy = 40 * np.sin(t * 0.0037 + 1.3)
    ddx, ddy = px - ox, py - oy
    rr = np.sqrt(ddx * ddx + ddy * ddy) + 2.0
    ang = np.arctan2(ddy, ddx)
    v = 850.0 / rr + t * 0.30                   # fly slowly down the tunnel
    u = ang * (8.0 / np.pi) + t * 0.004         # gentle spin
    tex = np.sin(u * np.pi) * np.sin(v * 0.32)  # soft checker walls
    sh = np.clip(rr / 55.0, 0.0, 1.0) ** 0.6    # depth darkening into the mouth
    hue = 0.52 + 0.11 * np.sin(v * 0.16) + 0.05 * tex + t * 0.00015
    val = sh * (0.42 + 0.50 * tex + 0.08)
    val = np.clip(val, 0.02, 1.0)
    sat = np.clip(0.72 - 0.25 * tex * sh, 0.0, 1.0)
    return (_hsv(hue, sat, val) * 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz013.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
