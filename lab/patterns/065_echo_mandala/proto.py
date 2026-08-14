"""065 Echo Mandala — closed-form feedback zoom: a flower stamp echoed at 8 rotated
scales, continuously diving so echoes rise from the center forever."""
import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx = _xx - W / 2.0
_dy = _yy - H / 2.0

SCALE = 1.9          # zoom step between echoes
ROT = 0.5            # radians of rotation per echo (the feedback "twist")
DECAY = 0.62
NOCT = 8


def _pal(stops):
    xs = np.array([s[0] for s in stops], np.float32)
    cs = np.array([s[1] for s in stops], np.float32)
    u = np.linspace(0.0, 1.0, 256)
    return np.stack([np.interp(u, xs, cs[:, i]) for i in range(3)], 1).astype(np.uint8)


# jewel loop: midnight -> emerald -> gold -> ruby -> violet -> midnight
PAL = _pal([
    (0.00, (8, 12, 40)), (0.20, (20, 160, 90)), (0.42, (250, 210, 70)),
    (0.62, (210, 40, 70)), (0.82, (130, 50, 200)), (1.00, (8, 12, 40)),
])


def render(t):
    z = t * 0.006                      # continuous dive
    frac = z % 1.0
    spin = t * 0.0012
    acc = np.zeros((H, W), np.float32)
    wsum = 0.0
    for k in range(NOCT):
        m = k - frac                   # echo depth, drifts smoothly
        sc = (SCALE ** m) * 0.021
        th = m * ROT + spin
        ca, sa = np.cos(th), np.sin(th)
        xr = (_dx * ca - _dy * sa) * sc
        yr = (_dx * sa + _dy * ca) * sc
        rr = np.hypot(xr, yr)
        aa = np.arctan2(yr, xr)
        g = np.cos(rr * 6.0) * np.cos(aa * 5.0 + rr * 1.5)   # 5-petal ring flower
        w = (DECAY ** m) * np.clip(m + 1.0, 0.0, 1.0)        # fade-in newborn echo
        acc += g * w
        wsum += w
    field = acc / wsum
    idx = np.mod(field * 120.0 + 128.0, 256.0).astype(np.uint8)
    return PAL[idx].copy()


if __name__ == "__main__":
    import os, subprocess
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_065.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
    print("wrote", out)
