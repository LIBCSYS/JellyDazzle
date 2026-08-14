"""061 Checker Tunnel — classic flying polar tunnel, rainbow spiral bands + checker shading."""
import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx = _xx - W / 2.0
_dy = _yy - H / 2.0
_r = np.hypot(_dx, _dy) + 1e-3
_a = np.arctan2(_dy, _dx)                      # -pi..pi
_ang = (_a / np.pi + 1.0) * 128.0              # 0..256, wraps
_depth = 5200.0 / (_r + 14.0)                  # 1/r depth illusion
_shade = _r / (_r + 60.0)                      # dark hole at the far end


def _pal(stops):
    xs = np.array([s[0] for s in stops], np.float32)
    cs = np.array([s[1] for s in stops], np.float32)
    u = np.linspace(0.0, 1.0, 256)
    return np.stack([np.interp(u, xs, cs[:, i]) for i in range(3)], 1).astype(np.uint8)


# looping rainbow (first == last)
PAL = _pal([
    (0.00, (140, 30, 200)), (0.16, (30, 60, 230)), (0.33, (20, 200, 190)),
    (0.50, (60, 220, 60)), (0.66, (250, 220, 40)), (0.83, (240, 70, 50)),
    (1.00, (140, 30, 200)),
])


def render(t):
    u = _ang * 3.0 + t * 0.12 + 6.0 * np.sin(_depth * 0.05 + t * 0.01)
    v = _depth + t * 0.9                       # fly forward
    idx = np.mod(u + v, 256.0).astype(np.uint8)
    checker = (np.mod(np.floor(u / 21.0) + np.floor(v / 21.0), 2.0) * 0.35 + 0.65)
    sh = (_shade * checker).astype(np.float32)
    rgb = PAL[idx].astype(np.float32) * sh[..., None]
    return rgb.astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_061.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
    print("wrote", out)
