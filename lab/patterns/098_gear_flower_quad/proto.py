"""098 gear_flower_quad — faithful replica of dazzle.exe R13 (frames d22-d25):
2x2 array of toothed gear/sunflower rosettes with dotted seed cores over diagonal
blue/cyan stripes; stepped triangle borders top and bottom; teeth rotate slowly
and the palette slowly swaps figure/ground (yellow-on-blue <-> blue-on-yellow)."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)

_CENTERS = ((80, 66), (240, 66), (80, 174), (240, 174))
_YEL = np.array((235, 220, 40), np.float32)
_GRN = np.array((120, 200, 40), np.float32)
_NVY = np.array((16, 30, 120), np.float32)
_CYN = np.array((40, 170, 230), np.float32)
_BLU = np.array((30, 60, 200), np.float32)


def render(t):
    img = np.empty((H, W, 3), np.float32)

    # slow figure/ground palette swap (d22-d23 yellow-on-blue -> d24 inverted)
    swap = 0.5 + 0.5 * np.sin(t * 0.004)
    c_disc_o = _YEL * (1 - swap) + _BLU * swap
    c_disc_i = _GRN * (1 - swap) + _CYN * swap
    c_gnd_a = _BLU * (1 - swap) + _YEL * swap
    c_gnd_b = _CYN * (1 - swap) + _GRN * swap
    c_core = _NVY * (1 - swap) + _YEL * swap
    c_dot = _YEL * (1 - swap) + _NVY * swap

    # --- diagonal stripe ground, drifting slowly
    s = np.sin((_xx + _yy) * 0.32 + t * 0.012)
    img[:] = np.where((s > 0)[..., None], c_gnd_a[None, None], c_gnd_b[None, None])

    # --- stepped triangle borders top/bottom
    for band, flip in ((_yy < 16, _yy), (_yy >= H - 16, H - 1 - _yy)):
        stepm = band & (((_xx + 5 * np.floor(flip / 4.0)) % 26) < 13)
        img[stepm] = _NVY * (1 - swap) + _YEL * swap

    # --- four gear-flower rosettes
    for cx, cy in _CENTERS:
        lx, ly = _xx - cx, _yy - cy
        rr = np.sqrt(lx * lx + ly * ly)
        aa = np.arctan2(ly, lx)
        tooth = 34 + 5.0 * np.cos(14 * (aa + t * 0.004))     # rotating teeth
        disc = rr < tooth
        rim = np.abs(rr - tooth) < 2.2
        u = np.clip(rr / 36.0, 0, 1)[..., None]
        disc_col = c_disc_i[None, None] * (1 - u) + c_disc_o[None, None] * u
        img = np.where(disc[..., None], disc_col, img)
        img[rim] = _NVY
        # dotted seed core: concentric dot rings
        core = rr < 15
        dots = (np.sin(rr * 1.35 - t * 0.02) > 0.35) & (np.sin(aa * 9 + rr * 0.6) > 0.1)
        img[core] = c_core
        img[core & dots] = c_dot

    # --- small mirrored motif chain on the center vertical
    axd = np.abs(_xx - W / 2)
    chain = (axd / 7.0 + np.abs(((_yy + t * 0.05) % 26) - 13) / 10.0) < 1.0
    img[chain] = c_dot

    return np.clip(img, 0, 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz098.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
