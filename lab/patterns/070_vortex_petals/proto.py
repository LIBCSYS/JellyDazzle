"""070 Vortex Petals — six petal arms spiraling into a dark core, tightness breathing;
the R16 pinwheel given gravitational pull."""
import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx = _xx - W / 2.0
_dy = _yy - H / 2.0
_r = np.hypot(_dx, _dy) + 1e-3
_a = np.arctan2(_dy, _dx)
_lr = np.log(_r + 3.0)
_depth = 2600.0 / (_r + 16.0)
_shade = _r / (_r + 30.0)


def _pal(stops):
    xs = np.array([s[0] for s in stops], np.float32)
    cs = np.array([s[1] for s in stops], np.float32)
    u = np.linspace(0.0, 1.0, 256)
    return np.stack([np.interp(u, xs, cs[:, i]) for i in range(3)], 1).astype(np.uint8)


# meadow-fire loop: deep moss -> green -> chartreuse -> gold -> vermilion -> maroon -> moss
PAL = _pal([
    (0.00, (10, 30, 12)), (0.18, (40, 140, 40)), (0.36, (150, 220, 50)),
    (0.54, (255, 200, 40)), (0.72, (230, 90, 30)), (0.88, (110, 25, 30)),
    (1.00, (10, 30, 12)),
])


def render(t):
    tw = 2.6 + 1.1 * np.sin(t * 0.005)          # spiral tightness breathes
    arms = np.sin(6.0 * (_a + _lr * tw * 0.5 - t * 0.008))   # 6 petal arms, rotating
    pull = np.sin(_depth * 0.5 + t * 0.42)      # matter streaming into the core
    rib = 0.3 * np.sin(_lr * 8.0 - t * 0.06)    # faint radial shading ribs
    field = arms * 0.62 + pull * 0.26 + rib
    idx = np.mod(field * 100.0 + 128.0, 256.0).astype(np.uint8)
    rgb = PAL[idx].astype(np.float32) * _shade[..., None]
    return rgb.astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_070.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
    print("wrote", out)
