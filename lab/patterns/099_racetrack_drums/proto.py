"""099 racetrack_drums — faithful replica of dazzle.exe R14 (frames d28-d30):
full-screen concentric stadium/racetrack rings in thick multicolor bands
shimmering outward; inside the innermost oval two striped drums roll vertically;
heavy palette cycling (acid green ground -> blue ground across the run)."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_ax = np.abs(_dx)

# stadium (racetrack) distance: distance to the horizontal core segment
_dxs = np.maximum(_ax - 64, 0.0)
_dstad = np.sqrt(_dxs * _dxs + _dy * _dy)

_BANDS = np.array([
    (150, 240, 30), (240, 230, 40), (30, 90, 220), (200, 40, 60),
    (60, 200, 180), (240, 130, 30), (40, 40, 160), (120, 230, 90),
], np.float32)

_DRUM = np.array([
    (40, 220, 60), (170, 240, 50), (250, 210, 40), (250, 120, 40),
    (230, 40, 50), (250, 120, 40), (250, 210, 40), (170, 240, 50),
], np.float32)


def render(t):
    img = np.empty((H, W, 3), np.float32)

    # --- thick quantized stadium bands, marching outward + hue-rolling
    band = np.floor((_dstad - t * 0.18) / 11.0).astype(np.int64)
    ci = (band + np.int64(t * 0.02)) % len(_BANDS)
    img[:] = _BANDS[ci]
    # band seams darkened for the thick-ring look
    seam = ((_dstad - t * 0.18) % 11.0) < 1.8
    img[seam] *= 0.35

    # --- inner oval court
    court = _dstad < 34
    img[court] = (14, 24, 90)

    # --- two side-by-side striped drums, stripes rolling vertically
    for x0, x1 in ((-56, -8), (8, 56)):
        drum = (_dx >= x0) & (_dx < x1) & (np.abs(_dy) < 26)
        di = np.floor((_yy - t * 0.55) / 6.0).astype(np.int64) % len(_DRUM)
        img = np.where(drum[..., None], _DRUM[di], img)
        dedge = drum & ((np.abs(_dx - x0) < 1.5) | (np.abs(_dx - x1) < 1.5))
        img[dedge] = (10, 10, 40)

    # --- small mirrored perimeter blobs riding a mid ring
    for bx, by in ((110, 62), (-110, 62), (110, -62), (-110, -62),
                   (0, 96), (0, -96), (140, 0), (-140, 0)):
        bb = ((_dx - bx) ** 2) / 90.0 + ((_dy - by) ** 2) / 55.0 < 1.0
        puls = 0.7 + 0.3 * np.sin(t * 0.02 + bx * 0.05 + by * 0.03)
        img[bb] = np.array((240, 60, 170), np.float32) * puls

    return np.clip(img, 0, 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz099.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
