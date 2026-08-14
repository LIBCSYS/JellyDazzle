"""091 diamond_confetti — faithful replica of dazzle.exe R1 (frame a01):
big centered rhombus, thick cyan/magenta rims, expanding concentric diamond
outlines, mirrored multicolor confetti interior, streaks off the side vertices."""
import numpy as np, os

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)
_dx, _dy = _xx - W / 2, _yy - H / 2
_ax, _ay = np.abs(_dx), np.abs(_dy)
# diamond (L1) norm, scaled so dd==1 on the rhombus rim (wide like a01)
_dd = _ax / 148.0 + _ay / 92.0

# per-cell hash for the confetti quilt (4-fold mirrored via |dx|,|dy|)
_cx = (_ax / 6.0).astype(np.int64)
_cy = (_ay / 6.0).astype(np.int64)
_h1 = (((_cx * 73856093) ^ (_cy * 19349663)) & 255).astype(np.float32)
_h2 = (((_cx * 83492791) ^ (_cy * 2971215073)) & 255).astype(np.float32)


def _hsv(h, s, v):
    """vectorized hsv->rgb, h/s/v arrays in [0,1] -> float rgb 0..255"""
    h6 = (h % 1.0) * 6.0
    i = np.floor(h6)
    f = h6 - i
    p, q, tt = v * (1 - s), v * (1 - s * f), v * (1 - s * (1 - f))
    i = i.astype(np.int32) % 6
    r = np.choose(i, [v, q, p, p, tt, v])
    g = np.choose(i, [tt, v, v, q, p, p])
    b = np.choose(i, [p, p, tt, v, v, q])
    return r * 255, g * 255, b * 255


_RIM = [(0, 255, 220), (255, 40, 255), (60, 255, 80)]  # cyan / magenta / green


def render(t):
    img = np.zeros((H, W, 3), np.float32)

    # --- confetti interior: each mirrored 6px cell drifts through the rainbow slowly
    hue = (_h1 / 255.0 + t * 0.0022 * (0.5 + _h2 / 255.0)) % 1.0
    sat = 0.75 + 0.25 * np.sin(_h2 * 0.13)
    val = 0.55 + 0.45 * np.sin(_h1 * 0.21 + t * 0.01 + _h2 * 0.05)
    r, g, b = _hsv(hue, np.clip(sat, 0, 1), np.clip(np.abs(val), 0, 1))
    inside = _dd < 0.88
    img[..., 0] = np.where(inside, r, 0)
    img[..., 1] = np.where(inside, g, 0)
    img[..., 2] = np.where(inside, b, 0)

    # --- expanding concentric diamond outlines born at the center
    ph = _dd * 5.0 - t * 0.016
    frac = ph % 1.0
    ring = (frac < 0.16) & (_dd < 0.88) & (_dd > 0.06)
    ridx = np.floor(ph).astype(np.int64) % 3
    fade = 0.35 + 0.65 * np.clip(_dd / 0.9, 0, 1)      # rings brighten as they grow
    for k, c in enumerate(_RIM):
        m = ring & (ridx == k)
        for ch in range(3):
            img[..., ch] = np.where(m, c[ch] * fade, img[..., ch])

    # --- thick static rims (cyan outer, magenta inner) with a slow glow pulse
    puls = 0.8 + 0.2 * np.sin(t * 0.02)
    rim1 = (_dd >= 0.94) & (_dd < 1.02)
    rim2 = (_dd >= 0.88) & (_dd < 0.94)
    for m, c, p in ((rim2, (255, 40, 255), 1.0), (rim1, (0, 255, 220), puls)):
        for ch in range(3):
            img[..., ch] = np.where(m, c[ch] * p, img[..., ch])

    # --- horizontal streaks escaping the left/right vertices
    streak = (_ay < 3.0) & (_dd >= 1.0)
    sph = 0.5 + 0.5 * np.sin(_ax * 0.20 - t * 0.05)
    for ch, c in enumerate((120, 255, 235)):
        img[..., ch] = np.where(streak, c * sph, img[..., ch])

    return np.clip(img, 0, 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz091.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
