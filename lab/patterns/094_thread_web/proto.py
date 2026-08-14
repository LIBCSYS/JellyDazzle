"""094 thread_web — faithful replica of dazzle.exe R4 (frames a08-a10):
purple flood; fine multicolor curved threads accumulate corner-to-center through
a 4-fold mirror, densifying into an X/butterfly web; rainbow wedge fans parked at
the side edges, green pinwheel triangles in the corners."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_ax, _ay = np.abs(_dx), np.abs(_dy)

PURPLE = np.array((88, 34, 200), np.float32)


def _hsv1(h, s=1.0, v=1.0):
    h6 = (h % 1.0) * 6.0
    i = int(h6) % 6
    f = h6 - int(h6)
    p, q, tt = v * (1 - s), v * (1 - s * f), v * (1 - s * (1 - f))
    r, g, b = [(v, tt, p), (q, v, p), (p, v, tt), (p, q, v), (tt, p, v), (v, p, q)][i]
    return np.array((r * 255, g * 255, b * 255), np.float32)


def _furniture(img):
    # rainbow wedge fans parked at left/right edge midline (mirrored)
    stripe = np.floor((_dy + _ax * 0.55) / 7.0).astype(np.int64) % 6
    fan = (_ax > 118) & (_ay < 0.45 * (_ax - 112))
    cols = np.array([(255, 40, 40), (255, 160, 20), (250, 240, 40),
                     (60, 220, 60), (50, 120, 255), (170, 60, 255)], np.float32)
    img[fan] = cols[stripe[fan]]
    # green pinwheel triangles hugging the corners
    tri = (_ax + _ay * 1.4 > 300) & ((_ax - _ay * 0.7) > 60)
    g = 120 + 90 * np.sin(_ax * 0.15)
    img[..., 0] = np.where(tri, 30, img[..., 0])
    img[..., 1] = np.where(tri, g, img[..., 1])
    img[..., 2] = np.where(tri, 60, img[..., 2])


_S = np.linspace(0.0, 1.0, 150, dtype=np.float32)


def render(t):
    img = np.empty((H, W, 3), np.float32)
    img[:] = PURPLE
    _furniture(img)

    # threads accumulate: ~1.2 new threads per frame, drawn mirrored 4-fold
    n = int(8 + t * 1.2)
    for i in range(n):
        rg = np.random.default_rng(9000 + i)
        # start near the top-left corner zone, end near center; bundle slowly rotates
        x0 = rg.uniform(0, 55)
        y0 = rg.uniform(0, 45)
        x1 = 160 - rg.uniform(0, 30)
        y1 = 120 - rg.uniform(0, 26)
        swing = np.sin(i * 0.05) * 30 + rg.uniform(-14, 14)
        px = x0 + (x1 - x0) * _S + swing * np.sin(_S * np.pi)
        py = y0 + (y1 - y0) * _S - swing * 0.7 * np.sin(_S * np.pi)
        ix = np.clip(px, 0, W // 2 - 1).astype(np.int64)
        iy = np.clip(py, 0, H // 2 - 1).astype(np.int64)
        c = _hsv1(rg.random(), 0.85, 1.0)
        # newest threads glow brighter; old ones settle into the web
        age = (n - i) / max(n, 1)
        cc = c * (0.55 + 0.45 * np.clip(1.5 - 3 * age, 0, 1))
        img[iy, ix] = cc
        img[iy, W - 1 - ix] = cc
        img[H - 1 - iy, ix] = cc
        img[H - 1 - iy, W - 1 - ix] = cc

    return np.clip(img, 0, 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz094.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
