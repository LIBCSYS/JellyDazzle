"""014 copper_octarings — Amiga copper bars promoted to concentric octagonal rings (octagonal norm), slowly breathing."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2


def render(t):
    th = t * 0.003                               # the octagon itself rotates, very slowly
    co, si = np.cos(th), np.sin(th)
    rx = _dx * co - _dy * si
    ry = _dx * si + _dy * co
    ax, ay = np.abs(rx), np.abs(ry)
    r8 = np.maximum(np.maximum(ax, ay), (ax + ay) * 0.70710678)  # octagonal norm
    col = np.zeros((H, W, 3), np.float32)
    # midnight backdrop with a faint slow ripple
    col[..., 0] = 10
    col[..., 1] = 6
    col[..., 2] = 30 + 16 * np.sin(r8 * 0.02 - t * 0.01)
    # six copper bars riding independent sine paths over radius
    bars = [
        (0.0, 0.0080, (255, 72, 72)),
        (1.1, 0.0065, (255, 192, 60)),
        (2.3, 0.0092, (96, 255, 128)),
        (3.4, 0.0071, (84, 162, 255)),
        (4.6, 0.0058, (214, 96, 255)),
        (5.5, 0.0086, (255, 244, 190)),
    ]
    for ph, w, (br, bg, bb) in bars:
        pos = 96 + 76 * np.sin(t * w + ph)       # ring radius sweeps 20..172
        wgt = np.clip(1.0 - np.abs(r8 - pos) / 17.0, 0, 1) ** 1.5
        col[..., 0] += wgt * br
        col[..., 1] += wgt * bg
        col[..., 2] += wgt * bb
    return np.clip(col, 0, 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz014.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
