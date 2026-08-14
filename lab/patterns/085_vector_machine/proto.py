import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)


def render(t):
    t = float(t)
    img = np.zeros((H, W, 3), np.float32)
    x = _xx - 160.0
    y = _yy - 120.0

    # red X lattice of struts behind everything
    for ang in (0.55, -0.55, 1.05, -1.05):
        d = np.abs(y - np.tan(ang) * x) * np.cos(ang)
        m = np.clip(1.0 - d / 2.0, 0, 1)
        img[..., 0] += m * 0.45
        img[..., 2] += m * 0.06

    # giant red concentric arc stacks left and right ("( )" parentheses)
    for sx in (-1.0, 1.0):
        dx = x - sx * 175.0
        r = np.hypot(dx, y)
        band = 0.5 + 0.5 * np.sin(r * 0.26 - t * 0.010 * sx)
        mask = (r > 55) & (r < 170) & (sx * dx < 0)
        arc = np.where(mask, band, 0.0)
        img[..., 0] += arc * 0.85
        img[..., 2] += arc * 0.12

    # central "H": two vertical gradient bars + crossbar (blue->magenta ramp rolls)
    barw, gap = 20.0, 40.0
    g = np.mod((y + 70.0) / 140.0 + t * 0.004, 1.0)
    for sx in (-1.0, 1.0):
        m = (np.abs(x - sx * gap) < barw) & (np.abs(y) < 72)
        img[..., 2] = np.where(m, 0.35 + 0.60 * g, img[..., 2])
        img[..., 0] = np.where(m, 0.20 + 0.60 * (1.0 - g), img[..., 0])
        img[..., 1] = np.where(m, 0.06, img[..., 1])
    gc = np.mod(x / 60.0 + t * 0.004, 1.0)
    mc = (np.abs(x) < gap) & (np.abs(y) < 13)
    img[..., 2] = np.where(mc, 0.35 + 0.60 * gc, img[..., 2])
    img[..., 0] = np.where(mc, 0.85 - 0.55 * gc, img[..., 0])
    img[..., 1] = np.where(mc, 0.10, img[..., 1])

    # small white diamond at dead center, breathing
    dman = np.abs(x) + np.abs(y)
    ds = 14.0 + 3.0 * np.sin(t * 0.02)
    dia = np.clip(1.0 - np.abs(dman - ds) / 2.0, 0, 1) + np.where(dman < ds, 0.75, 0.0)
    img += dia[..., None] * np.array([0.9, 0.85, 0.95], np.float32) * 0.9
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os
    import subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_085.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
