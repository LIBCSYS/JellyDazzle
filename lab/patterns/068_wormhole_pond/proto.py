"""068 Wormhole Pond — a breathing organic wormhole crossed with a drifting ripple
source; the R19 interference pond given true depth."""
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


# pond loop: black-green -> jade -> spring green -> white foam -> deep blue -> black-green
PAL = _pal([
    (0.00, (4, 22, 14)), (0.22, (20, 130, 80)), (0.44, (90, 220, 110)),
    (0.58, (225, 255, 235)), (0.74, (30, 90, 160)), (0.90, (10, 40, 70)),
    (1.00, (4, 22, 14)),
])


def render(t):
    # organic wobble of the throat: 2- and 3-lobed, slowly counter-rotating
    wob = (1.0 + 0.22 * np.sin(2.0 * _a + t * 0.012)
               + 0.12 * np.sin(3.0 * _a - t * 0.008))
    rr = _r * wob
    depth = 2000.0 / (rr + 6.0)
    # drifting secondary ripple source
    sx = 70.0 * np.sin(t * 0.0035 + 0.9)
    sy = 50.0 * np.sin(t * 0.0052 + 2.2)
    r2 = np.hypot(_dx - sx, _dy - sy)
    field = (np.sin(depth * 0.40 - t * 0.35)          # rings pouring into the throat
             + 0.8 * np.sin(rr * 0.16 - t * 0.22)     # surface swell radiating out
             + 0.6 * np.sin(r2 * 0.20 - t * 0.30))    # drifting rain-drop ripples
    idx = np.mod(field * 74.0 + 128.0, 256.0).astype(np.uint8)
    shade = np.clip(rr / (rr + 34.0) * 1.15, 0.0, 1.0)
    rgb = PAL[idx].astype(np.float32) * shade[..., None]
    return rgb.astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_068.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
    print("wrote", out)
