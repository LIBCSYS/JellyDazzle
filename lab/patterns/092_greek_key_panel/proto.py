"""092 greek_key_panel — faithful replica of dazzle.exe R2 (frames a02-a04):
static greek-key maze panel + blue wedge border + horizontal rays; the GEOMETRY
never moves, the whole palette rotates (yellow -> red-on-blue -> blue phases)."""
import numpy as np, os

W, H = 320, 240

# ---------------- static index map (built once, like the original's framebuffer)
_yy, _xx = np.mgrid[0:H, 0:W]
_dx, _dy = _xx - W / 2.0, _yy - H / 2.0
_ax, _ay = np.abs(_dx), np.abs(_dy)

# greek-key / meander motif, 12x12 (1 = key line)
_KEY = np.array([[int(c) for c in row] for row in [
    "111111111111",
    "100000000000",
    "101111111101",
    "101000000101",
    "101011110101",
    "101010010101",
    "101010110101",
    "101010000101",
    "101011111101",
    "101000000001",
    "101111111111",
    "100000000000",
]], np.uint8)
_KEY = np.kron(_KEY, np.ones((2, 2), np.uint8))          # 24x24 px per motif

IDX = np.zeros((H, W), np.uint8)                          # class map
# class 0 background, 1 panel ground, 2 key lines, 3 wedges, 4 rosettes, 5 rays

# horizontal rays behind everything (fan converging on center, near-horizontal)
ang = np.arctan2(_ay, _ax + 0.001)                        # 0..pi/2, mirrored
raym = ((ang * 40 / (np.pi / 2)) % 1.0 < 0.30) & (ang < 0.42)
IDX[raym] = 5

# blue kaleidoscope wedge border (angular wedges outside the panel)
wed = ((np.arctan2(_dy, _dx) / (2 * np.pi) * 24) % 1.0 < 0.5)
border = (_ax < 132) & (_ay < 92) & ~((_ax < 116) & (_ay < 76))
IDX[border & wed] = 3
IDX[border & ~wed] = 0

# central panel: mirrored greek-key tiling
panel = (_ax < 116) & (_ay < 76)
kx = (_ax.astype(np.int64)) % 24
ky = (_ay.astype(np.int64)) % 24
keyline = _KEY[ky, kx] == 1
IDX[panel] = np.where(keyline[panel], 2, 1)

# small 6-lobed rosettes along the top/bottom border line
for rx in (-96, -48, 0, 48, 96):
    for ry in (-84, 84):
        rr = np.sqrt((_dx - rx) ** 2 + (_dy - ry) ** 2)
        ra = np.arctan2(_dy - ry, _dx - rx)
        ros = (rr < 7 + 3.2 * np.cos(6 * ra))
        IDX[ros] = 4

# ---------------- rolling palette (geometry static, DAC rotates: a02->a03->a04)
_STOPS = np.array([
    (255, 230, 40), (255, 150, 20), (230, 40, 40), (200, 20, 120),
    (60, 30, 220), (20, 80, 255), (30, 200, 200), (120, 230, 60),
], np.float32)


def _wheel(idx256):
    """smooth looping palette lookup, idx256 float array -> rgb"""
    p = (idx256 % 256.0) / 256.0 * len(_STOPS)
    i0 = np.floor(p).astype(np.int64) % len(_STOPS)
    i1 = (i0 + 1) % len(_STOPS)
    f = (p - np.floor(p))[..., None]
    return _STOPS[i0] * (1 - f) + _STOPS[i1] * f


# fixed offsets keep class relationships constant while the whole wheel rolls
_OFF = {0: 150.0, 1: 0.0, 2: 96.0, 3: 170.0, 4: 210.0, 5: 40.0}
_DIM = {0: 0.45, 1: 0.95, 2: 0.85, 3: 0.9, 4: 1.0, 5: 0.7}


def render(t):
    roll = t * 0.35                       # slow DAC rotation
    img = np.zeros((H, W, 3), np.float32)
    for cls in range(6):
        m = IDX == cls
        if not m.any():
            continue
        base = np.full(m.sum(), _OFF[cls] + roll, np.float32)
        if cls == 5:                       # rays shimmer along their length
            base += (_ax[m] * 0.5)
        if cls == 3:                       # wedges shade with angle
            base += (np.arctan2(_dy, _dx)[m] * 18.0)
        img[m] = _wheel(base) * _DIM[cls]
    return np.clip(img, 0, 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz092.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
