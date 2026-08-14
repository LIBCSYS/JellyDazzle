"""062 Twist Corridor — square corridor receding to center, walls twisting with depth."""
import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx = _xx - W / 2.0
_dy = _yy - H / 2.0
_r = np.hypot(_dx, _dy) + 1e-3


def _pal(stops):
    xs = np.array([s[0] for s in stops], np.float32)
    cs = np.array([s[1] for s in stops], np.float32)
    u = np.linspace(0.0, 1.0, 256)
    return np.stack([np.interp(u, xs, cs[:, i]) for i in range(3)], 1).astype(np.uint8)


# cool ice loop: navy -> blue -> cyan -> white -> violet -> navy
PAL = _pal([
    (0.00, (10, 16, 70)), (0.22, (30, 70, 220)), (0.45, (40, 210, 240)),
    (0.60, (235, 250, 255)), (0.80, (150, 70, 220)), (1.00, (10, 16, 70)),
])


def render(t):
    # twist angle grows toward the center (deep end) -> corridor appears to torque
    phi = t * 0.0009 * (1.0 + 70.0 / (_r + 28.0))
    ca, sa = np.cos(phi), np.sin(phi)
    rx = np.abs(_dx * ca - _dy * sa)
    ry = np.abs(_dx * sa + _dy * ca)
    d = np.maximum(rx, ry)                    # square (Chebyshev) norm -> nested squares
    depth = 2400.0 / (d + 10.0)
    v = depth * 1.7 + t * 0.8                 # rush forward
    # wall sector hue (4 walls of the rotated square)
    sector = (np.arctan2(_dy, _dx) + phi) * (2.0 / np.pi)
    sector = np.mod(np.floor(sector + 0.5), 4.0)
    idx = np.mod(v + sector * 32.0, 256.0).astype(np.uint8)
    glow = 0.72 + 0.28 * np.sin(depth * 0.5 + t * 0.06)
    shade = d / (d + 38.0)
    rgb = PAL[idx].astype(np.float32) * (glow * shade)[..., None]
    return rgb.astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_062.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
    print("wrote", out)
