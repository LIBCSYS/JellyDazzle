"""064 Starburst Forge — sweeping fire rays crossed with radial copper rings flying inward."""
import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx = _xx - W / 2.0
_dy = _yy - H / 2.0
_r = np.hypot(_dx, _dy) + 1e-3
_a = np.arctan2(_dy, _dx)
_depth = 2200.0 / (_r + 12.0)
_shade = _r / (_r + 42.0)


def _pal(stops):
    xs = np.array([s[0] for s in stops], np.float32)
    cs = np.array([s[1] for s in stops], np.float32)
    u = np.linspace(0.0, 1.0, 256)
    return np.stack([np.interp(u, xs, cs[:, i]) for i in range(3)], 1).astype(np.uint8)


# forge loop: near-black -> ember red -> orange -> gold -> white -> gold -> red -> near-black
PAL = _pal([
    (0.00, (12, 4, 10)), (0.18, (140, 20, 25)), (0.36, (235, 90, 20)),
    (0.52, (255, 190, 40)), (0.62, (255, 250, 220)), (0.72, (255, 190, 40)),
    (0.86, (150, 30, 30)), (1.00, (12, 4, 10)),
])


def render(t):
    sweep = t * 0.004
    rays = np.sin(_a * 12.0 + sweep + 0.55 * np.sin(_depth * 0.045 + t * 0.008))
    rings = np.sin(_depth * 0.45 - t * 0.5)   # copper rings racing to the center
    field = rays * 0.62 + rings * 0.38
    idx = np.mod(field * 118.0 + 128.0, 256.0).astype(np.uint8)
    rgb = PAL[idx].astype(np.float32) * _shade[..., None]
    return rgb.astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_064.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
    print("wrote", out)
