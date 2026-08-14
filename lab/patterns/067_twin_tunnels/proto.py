"""067 Twin Tunnels — two orbiting tunnel mouths inside a 4-fold kaleidoscope fold,
their rings interfering into moire lenses at the seams."""
import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
# fold into one quadrant (4-fold mirror) BEFORE placing the tunnel centers
_fx = np.abs(_xx - W / 2.0)
_fy = np.abs(_yy - H / 2.0)


def _pal(stops):
    xs = np.array([s[0] for s in stops], np.float32)
    cs = np.array([s[1] for s in stops], np.float32)
    u = np.linspace(0.0, 1.0, 256)
    return np.stack([np.interp(u, xs, cs[:, i]) for i in range(3)], 1).astype(np.uint8)


# deep-sea loop: abyss blue -> teal -> seafoam -> magenta -> abyss
PAL = _pal([
    (0.00, (6, 18, 55)), (0.24, (10, 150, 160)), (0.46, (130, 240, 200)),
    (0.64, (230, 120, 210)), (0.84, (90, 30, 130)), (1.00, (6, 18, 55)),
])


def render(t):
    # two mouths on slow Lissajous orbits within the quadrant
    ox1 = 88.0 + 30.0 * np.sin(t * 0.0040)
    oy1 = 62.0 + 24.0 * np.sin(t * 0.0031 + 1.7)
    ox2 = 40.0 + 26.0 * np.sin(t * 0.0026 + 0.6)
    oy2 = 34.0 + 20.0 * np.sin(t * 0.0047 + 3.1)
    r1 = np.hypot(_fx - ox1, _fy - oy1) + 1e-3
    r2 = np.hypot(_fx - ox2, _fy - oy2) + 1e-3
    d1 = 1900.0 / (r1 + 9.0)
    d2 = 1900.0 / (r2 + 9.0)
    field = (np.sin(d1 * 0.5 + t * 0.40)
             + np.sin(d2 * 0.5 - t * 0.33)
             + 0.55 * np.sin((r1 - r2) * 0.09))
    idx = np.mod(field * 88.0 + 128.0, 256.0).astype(np.uint8)
    shade = (r1 / (r1 + 40.0)) * (r2 / (r2 + 40.0)) * 1.35
    rgb = PAL[idx].astype(np.float32) * np.clip(shade, 0.0, 1.0)[..., None]
    return rgb.astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_067.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
    print("wrote", out)
