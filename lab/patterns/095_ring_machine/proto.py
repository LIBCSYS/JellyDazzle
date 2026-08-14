"""095 ring_machine — faithful replica of dazzle.exe R7 (frames c01-c05):
red concentric arc stacks left and right like giant parentheses, blue-magenta
gradient rectangles forming the central H, a red X-lattice of struts behind,
dim green scanlines in the outer field, white diamond at dead center."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_ax, _ay = np.abs(_dx), np.abs(_dy)

_rL = np.sqrt((_xx - 108) ** 2 + (_yy - 120) ** 2)
_rR = np.sqrt((_xx - 212) ** 2 + (_yy - 120) ** 2)


def render(t):
    img = np.zeros((H, W, 3), np.float32)

    # --- dim green horizontal scanlines in the outer field
    scan = ((_yy.astype(np.int64) % 4) == 0)
    g = 55 + 25 * np.sin(_xx * 0.05 + t * 0.01)
    img[..., 1] = np.where(scan, g, 0)

    # --- red X lattice of straight struts through center (behind everything)
    for a in (0.28, 0.55, 0.85):
        d = np.abs(_ay - _ax * a) / np.sqrt(1 + a * a)
        strut = d < 1.4
        img[..., 0] = np.where(strut, 150, img[..., 0])
        img[..., 1] = np.where(strut, 20, img[..., 1])
        img[..., 2] = np.where(strut, 30, img[..., 2])

    # --- red concentric arc stacks: "(" left, ")" right, phase creeping
    for rr, cx, opendir in ((_rL, 108, -1), (_rR, 212, 1)):
        ring = np.sin(rr * 0.55 - t * 0.02) > 0.15
        ann = (rr > 26) & (rr < 108)
        side = (_xx - cx) * opendir > 8          # keep only outward-bulging arcs
        m = ring & ann & side
        shade = 0.55 + 0.45 * np.sin(rr * 0.55 - t * 0.02 + 1.3)
        img[..., 0] = np.where(m, 255 * shade, img[..., 0])
        img[..., 1] = np.where(m, 30 * shade, img[..., 1])
        img[..., 2] = np.where(m, 45 * shade, img[..., 2])

    # --- blue->magenta gradient rectangles (top and bottom of the H), rolling
    for y0, y1 in ((36, 102), (138, 204)):
        rect = (_xx >= 96) & (_xx < 224) & (_yy >= y0) & (_yy < y1)
        u = (_yy - y0) / float(y1 - y0)
        ph = u * 2.4 + np.sin(t * 0.008) * 0.8
        rr = 90 + 150 * np.clip(np.sin(ph), 0, 1)
        bb = 255 - 60 * np.clip(np.sin(ph), 0, 1)
        img[..., 0] = np.where(rect, rr, img[..., 0])
        img[..., 1] = np.where(rect, 30, img[..., 1])
        img[..., 2] = np.where(rect, bb, img[..., 2])

    # --- red crossbar of the H with slow pulse, white diamond at dead center
    bar = (_xx >= 96) & (_xx < 224) & (_ay < 11)
    p = 0.75 + 0.25 * np.sin(t * 0.015)
    img[..., 0] = np.where(bar, 235 * p, img[..., 0])
    img[..., 1] = np.where(bar, 25, img[..., 1])
    img[..., 2] = np.where(bar, 55, img[..., 2])
    dia = (_ax + _ay) < 13
    core = (_ax + _ay) < 6
    img[dia] = (255, 70, 90)
    img[core] = (255, 245, 250)

    return np.clip(img, 0, 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz095.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
