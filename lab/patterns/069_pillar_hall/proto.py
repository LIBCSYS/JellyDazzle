"""069 Pillar Hall — a checkered marble corridor with dark pillars marching past,
true four-plane perspective (floor, ceiling, two walls) with gentle camera sway."""
import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
ASPECT = 4.0 / 3.0


def _pal(stops):
    xs = np.array([s[0] for s in stops], np.float32)
    cs = np.array([s[1] for s in stops], np.float32)
    u = np.linspace(0.0, 1.0, 256)
    return np.stack([np.interp(u, xs, cs[:, i]) for i in range(3)], 1).astype(np.uint8)


# amber hall loop: near-black -> mahogany -> amber -> gold -> cream -> mahogany -> near-black
PAL = _pal([
    (0.00, (16, 8, 4)), (0.20, (110, 45, 20)), (0.42, (220, 130, 30)),
    (0.58, (255, 205, 90)), (0.70, (255, 240, 200)), (0.86, (120, 55, 30)),
    (1.00, (16, 8, 4)),
])


def render(t):
    sway = 16.0 * np.sin(t * 0.004)
    bob = 7.0 * np.sin(t * 0.0027)
    dx = _xx - (W / 2.0 + sway)
    dy = _yy - (H / 2.0 + bob)
    adx = np.abs(dx) + 1e-3
    ady = np.abs(dy) * ASPECT + 1e-3
    wall = adx > ady                            # side walls vs floor/ceiling
    # perspective depth per plane
    depth = np.where(wall, 2400.0 / adx, 2400.0 / ady)
    depth = np.minimum(depth, 420.0)
    # lateral texture coordinate on each plane
    u = np.where(wall, dy * ASPECT / adx, dx / ady) * 34.0
    v = depth * 0.55 + t * 0.8                  # march forward
    tile = np.mod(np.floor(u / 16.0) + np.floor(v / 20.0), 2.0)
    # pillars: dark vertical bands on the walls at regular corridor intervals
    pil = wall & (np.mod(v, 90.0) < 16.0)
    idx = np.mod(depth * 0.55 + tile * 46.0 + np.where(wall, 36.0, 0.0), 256.0)
    idx = idx.astype(np.uint8)
    fog = np.clip(1.0 - depth / 460.0, 0.0, 1.0)        # dark at the far end
    lum = fog * (0.55 + 0.45 * tile)
    lum = np.where(pil, fog * 0.22, lum)
    rgb = PAL[idx].astype(np.float32) * lum[..., None]
    return rgb.astype(np.uint8)


if __name__ == "__main__":
    import os, subprocess
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_069.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
    print("wrote", out)
