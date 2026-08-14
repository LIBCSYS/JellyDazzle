"""096 scanline_butterfly — faithful replica of dazzle.exe R9 (frames c10-c13):
full-field horizontal scanlines whose spacing warps around two wing lobes,
forming a green moire butterfly with a cyan diamond heart and a red spindle;
red/white ring ornaments in the corners. Bands crawl slowly."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_ax, _ay = np.abs(_dx), np.abs(_dy)

# wing lobe field: distance to wing cores at (+-78, 0)
_dwing = np.sqrt((_ax - 78) ** 2 + (_dy * 1.15) ** 2)
_wing = np.exp(-_dwing / 68.0)                     # 1 at wing core -> 0 far away
# central diamond heart + spindle
_heart = (_ax / 46.0 + _ay / 62.0) < 1.0
_spindle = (_ax < 9.0 * np.clip(1 - _ay / 46.0, 0, 1)) & (_ay < 46)
_dcorn = np.sqrt((160 - _ax) ** 2 + (120 - _ay) ** 2)


def render(t):
    img = np.zeros((H, W, 3), np.float32)

    # scanlines warped by the wing field -> moire interference
    lines = np.sin(_yy * 0.85 + 38.0 * _wing + t * 0.022)
    lit = lines > -0.15
    bright = np.clip((lines + 0.15) / 1.15, 0, 1)

    # wing color ramp: dark green -> green -> yellow -> white toward wing cores
    wv = _wing
    r = np.where(wv < 0.5, wv * 2 * 190, 190 + (wv - 0.5) * 2 * 65)
    g = 70 + 185 * np.clip(wv * 2.1, 0, 1)
    b = np.where(wv > 0.75, (wv - 0.75) * 4 * 200, 20)
    img[..., 0] = np.where(lit, r * bright, 6)
    img[..., 1] = np.where(lit, g * bright, 14)
    img[..., 2] = np.where(lit, b * bright, 6)

    # cyan diamond heart (still scanlined, so it shimmers with the field)
    img[..., 0] = np.where(_heart, np.where(lit, 40 * bright, 4), img[..., 0])
    img[..., 1] = np.where(_heart, np.where(lit, 220 * bright, 10), img[..., 1])
    img[..., 2] = np.where(_heart, np.where(lit, 235 * bright, 14), img[..., 2])

    # red spindle at dead center
    img[..., 0] = np.where(_spindle, np.where(lit, 255 * (0.6 + 0.4 * bright), 60), img[..., 0])
    img[..., 1] = np.where(_spindle, 30 * bright, img[..., 1])
    img[..., 2] = np.where(_spindle, 40 * bright, img[..., 2])

    # corner ornaments: red/white concentric rings, breathing slowly
    orn = _dcorn < 46
    ringp = np.sin(_dcorn * 0.55 - t * 0.017)
    red = ringp > 0.2
    wht = ringp < -0.55
    img[..., 0] = np.where(orn & red, 235, img[..., 0])
    img[..., 1] = np.where(orn & red, 40, img[..., 1])
    img[..., 2] = np.where(orn & red, 50, img[..., 2])
    img[..., 0] = np.where(orn & wht, 245, img[..., 0])
    img[..., 1] = np.where(orn & wht, 240, img[..., 1])
    img[..., 2] = np.where(orn & wht, 225, img[..., 2])

    return np.clip(img, 0, 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz096.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
