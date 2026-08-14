"""066 Hex Tunnel — neon nested hexagons receding into darkness, slowly spinning."""
import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx = _xx - W / 2.0
_dy = _yy - H / 2.0
_r = np.hypot(_dx, _dy) + 1e-3
_a = np.arctan2(_dy, _dx)


def _pal(stops):
    xs = np.array([s[0] for s in stops], np.float32)
    cs = np.array([s[1] for s in stops], np.float32)
    u = np.linspace(0.0, 1.0, 256)
    return np.stack([np.interp(u, xs, cs[:, i]) for i in range(3)], 1).astype(np.uint8)


# neon loop: deep violet -> magenta -> hot cyan -> white -> violet
PAL = _pal([
    (0.00, (28, 6, 55)), (0.25, (190, 30, 170)), (0.50, (30, 220, 255)),
    (0.68, (240, 250, 255)), (0.85, (110, 40, 190)), (1.00, (28, 6, 55)),
])


def render(t):
    th = t * 0.0015                            # slow hex rotation
    # hexagonal norm: max of |projection| onto three axes 60 deg apart
    d = None
    for j in range(3):
        ax = th + j * (np.pi / 3.0)
        p = np.abs(_dx * np.cos(ax) + _dy * np.sin(ax))
        d = p if d is None else np.maximum(d, p)
    depth = np.minimum(2400.0 / (d + 8.0), 300.0)
    ring = 0.5 + 0.5 * np.sin(depth * 0.5 + t * 0.45)
    ring = ring ** 3                           # sharpen into neon hoops
    vert = 0.5 + 0.5 * np.cos(6.0 * (_a - th))  # corner glow at hex vertices
    idx = np.mod(depth * 0.9 + t * 0.5, 256.0).astype(np.uint8)
    shade = d / (d + 46.0)
    lum = (0.22 + 0.78 * ring) * (0.72 + 0.28 * vert) * shade
    rgb = PAL[idx].astype(np.float32) * lum[..., None]
    return rgb.astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_066.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
    print("wrote", out)
