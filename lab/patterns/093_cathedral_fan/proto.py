"""093 cathedral_fan — faithful replica of dazzle.exe R3 (frames a05-a07, f mid-left tile):
red/pink ray fan pinched at a central horizon, stair-stepped spires growing above
and below the centerline, concentric arc sets pulsing in the corners."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_ax, _ay = np.abs(_dx), np.abs(_dy)
_ang = np.arctan2(_ay, _ax + 0.001)          # 0..pi/2 mirrored quadrant angle
_r = np.sqrt(_dx * _dx + _dy * _dy)
_dcorn = np.sqrt((160 - _ax) ** 2 + (120 - _ay) ** 2)   # distance to corners

# spires: (x offset from center, half width, max height, growth phase)
_SPIRES = ((36, 15, 96, 0.0), (78, 12, 74, 0.3), (116, 9, 52, 0.6))


def render(t):
    img = np.zeros((H, W, 3), np.float32)

    # --- ray fan converging on the center, dense near the horizon
    sweep = t * 0.006
    rayph = (_ang * 30 / (np.pi / 2) + sweep) % 1.0
    raymask = rayph < 0.42
    pinch = (1.0 - _ang / (np.pi / 2)) ** 2.2          # bright near horizontal
    depth = np.clip(_r / 170.0, 0, 1)                  # white hot far out
    ri = pinch * (0.35 + 0.65 * depth)
    gold = (np.floor((_ang * 30 / (np.pi / 2) + sweep)) % 5) == 0
    img[..., 0] += np.where(raymask, 255 * ri, 0)
    img[..., 1] += np.where(raymask, np.where(gold, 200, 70) * ri, 0)
    img[..., 2] += np.where(raymask, np.where(gold, 60, 110) * ri, 0)

    # --- glowing horizon line
    hor = np.exp(-_ay * _ay / 6.0)
    img[..., 0] += 255 * hor
    img[..., 1] += 220 * hor
    img[..., 2] += 160 * hor

    # --- stair-stepped spires rise above/below the horizon (mirrored 4-fold)
    grow = np.clip(t / 700.0, 0.0, 1.0)
    for sx, w0, hmax, phz in _SPIRES:
        hnow = hmax * np.clip(grow - phz, 0, 1) / max(1e-6, 1 - phz)
        hnow = min(hmax, 8 + hnow)
        step = np.floor(_ay / 10.0)                     # 10-px stair steps
        wid = w0 - step * (w0 * 9.0 / hmax)             # shrink per step
        body = (np.abs(_ax - sx) < wid) & (_ay < hnow)
        edge = body & ((_ay % 10) < 2)                  # step seams
        shade = 1.0 - 0.55 * (_ay / max(hmax, 1))
        img[..., 0] = np.where(body, 70 * shade, img[..., 0])
        img[..., 1] = np.where(body, 40 * shade, img[..., 1])
        img[..., 2] = np.where(body, 230 * shade, img[..., 2])
        img[..., 0] = np.where(edge, 60, img[..., 0])
        img[..., 1] = np.where(edge, 240, img[..., 1])
        img[..., 2] = np.where(edge, 90, img[..., 2])

    # --- concentric arc sets breathing in the four corners
    arc = (np.sin(_dcorn * 0.42 - t * 0.03) > 0.55) & (_dcorn < 78)
    afade = np.clip(1 - _dcorn / 78.0, 0, 1)
    img[..., 0] = np.where(arc, 240 * afade + img[..., 0] * 0.2, img[..., 0])
    img[..., 1] = np.where(arc, 190 * afade, img[..., 1])
    img[..., 2] = np.where(arc, 60 * afade, img[..., 2])

    return np.clip(img, 0, 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz093.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
