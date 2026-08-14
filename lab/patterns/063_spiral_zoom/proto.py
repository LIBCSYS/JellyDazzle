"""063 Spiral Zoom — infinite self-similar log-spiral zoom, two interlocked chiralities."""
import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx = _xx - W / 2.0
_dy = _yy - H / 2.0
_r = np.hypot(_dx, _dy)
_a = np.arctan2(_dy, _dx)
_lr = np.log(_r + 2.0) * 82.0                 # log-radius: zoom == translation
K1 = 3 * 256.0 / (2.0 * np.pi)                # 3-arm spiral (wrap-safe)
K2 = 2 * 256.0 / (2.0 * np.pi)                # 2-arm counter-spiral
_shade = _r / (_r + 26.0)


def _pal(stops):
    xs = np.array([s[0] for s in stops], np.float32)
    cs = np.array([s[1] for s in stops], np.float32)
    u = np.linspace(0.0, 1.0, 256)
    return np.stack([np.interp(u, xs, cs[:, i]) for i in range(3)], 1).astype(np.uint8)


# royal loop: deep purple -> gold -> teal -> midnight -> purple
PAL = _pal([
    (0.00, (60, 10, 90)), (0.20, (200, 60, 160)), (0.38, (255, 200, 60)),
    (0.55, (30, 200, 170)), (0.75, (10, 40, 90)), (1.00, (60, 10, 90)),
])


def render(t):
    f1 = _lr * 2.4 - t * 1.1 + _a * K1        # zooming-in 3-arm spiral
    f2 = _lr * 1.1 + t * 0.5 - _a * K2        # slower counter-rotating 2-arm
    field = (np.sin(f1 * np.pi / 128.0) + np.sin(f2 * np.pi / 128.0)) * 63.0 + 128.0
    idx = np.mod(field, 256.0).astype(np.uint8)
    rgb = PAL[idx].astype(np.float32) * _shade[..., None]
    return rgb.astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_063.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
    print("wrote", out)
