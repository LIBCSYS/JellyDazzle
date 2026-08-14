"""018 kefrens_spiral — Kefrens bars run over RADIUS instead of scanlines: taffy tendrils melting outward, 4-fold mirrored."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_r = np.sqrt(_dx * _dx + _dy * _dy)
_a = np.arctan2(_dy, _dx)

N = 4
_W2 = 2 * np.pi / N            # wedge; folded half-wedge is _W2/2
_HW = _W2 * 0.5
_fa = np.mod(_a, _W2)
_fa = np.where(_fa > _HW, _W2 - _fa, _fa)

ABINS = 360                    # angle bins across the folded half-wedge
RMAX = 205
_BIN = np.clip((_fa / _HW) * (ABINS - 1), 0, ABINS - 1).astype(np.int32)
_RI = np.clip(_r, 0, RMAX - 1).astype(np.int32)


def _hsv(h, s, v):
    h = np.mod(h, 1.0) * 6.0
    i = np.floor(h).astype(np.int32) % 6
    f = h - np.floor(h)
    p, q, tt = v * (1 - s), v * (1 - s * f), v * (1 - s * (1 - f))
    r = np.choose(i, [v, q, p, p, tt, v])
    g = np.choose(i, [tt, v, v, q, p, p])
    b = np.choose(i, [p, p, tt, v, v, q])
    return np.stack([r, g, b], -1)


def render(t):
    bins = np.arange(ABINS, dtype=np.float32)
    melt = np.zeros(ABINS, np.float32)          # the single Kefrens line buffer (never cleared)
    mhue = np.zeros(ABINS, np.float32)
    rows_v = np.zeros((RMAX, ABINS), np.float32)
    rows_h = np.zeros((RMAX, ABINS), np.float32)
    for r in range(RMAX):
        # bar angle wanders with radius and time (two sines, classic Kefrens motion)
        th = 2.2 * np.sin(r * 0.045 + t * 0.012) + 1.4 * np.sin(r * 0.023 - t * 0.007) + t * 0.004
        fth = th % _W2
        if fth > _HW:
            fth = _W2 - fth                      # bar bounces inside the folded wedge
        cb = (fth / _HW) * (ABINS - 1)
        d = np.abs(bins - cb)
        bump = np.clip(1.0 - d / 15.0, 0, 1)
        m = bump > 0.03
        melt = np.where(m, np.maximum(melt, bump), melt * 0.9865)   # bar stamps, rest melts away
        mhue = np.where(m & (bump > 0.55), (r * 0.004 + t * 0.0011) % 1.0, mhue)
        rows_v[r] = melt
        rows_h[r] = mhue
    val = rows_v[_RI, _BIN]
    hue = rows_h[_RI, _BIN]
    v = np.clip(val, 0, 1) ** 0.85 * (0.45 + 0.55 * np.clip(1.0 - _r / 230.0, 0, 1))
    v = np.clip(v + 0.03, 0, 1)                 # faint deep-violet floor, never pure black
    sat = np.clip(0.9 - val * 0.25, 0.3, 1.0)
    return (_hsv(hue, sat, v) * 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz018.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
