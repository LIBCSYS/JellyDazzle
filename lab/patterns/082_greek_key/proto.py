import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)


def pal(v):
    a = np.array([0.5, 0.5, 0.5], np.float32)
    b = np.array([0.5, 0.5, 0.5], np.float32)
    c = np.array([1.0, 1.0, 1.0], np.float32)
    d = np.array([0.05, 0.38, 0.70], np.float32)
    return np.clip(a + b * np.cos(2 * np.pi * (c * v[..., None] + d)), 0, 1)


def render(t):
    t = float(t)
    px = _xx - 160.0
    py = _yy - 120.0

    # ---- central greek-key maze panel (240x160), grid of square-spiral hooks ----
    L = 40.0
    inpanel = (np.abs(px) < 120) & (np.abs(py) < 80)
    gx = np.floor((px + 120) / L)
    gy = np.floor((py + 80) / L)
    u = (px + 120) - gx * L - L / 2
    v = (py + 80) - gy * L - L / 2
    chir = np.where((gx + gy) % 2 == 0, 1.0, -1.0).astype(np.float32)
    un = u / (L / 2)
    vn = v / (L / 2) * chir                       # alternate chirality interlocks keys
    r = np.maximum(np.abs(un), np.abs(vn))
    a = np.arctan2(vn, un) / (2 * np.pi)          # -0.5..0.5
    s = r * 3.0 - a + t * 0.004                   # square spiral, slow crawl
    key = np.mod(s, 1.0) < 0.5
    cellhue = np.sin(gx * 1.3 + gy * 2.1) * 0.06
    hue_panel = np.where(key, 0.02, 0.55) + cellhue + t * 0.0015  # palette-cycled

    # ---- border: kaleidoscope wedges + radial shimmer ----
    th = np.arctan2(py, px)
    wed = np.mod(th * 8 / (2 * np.pi) + t * 0.003, 1.0)
    dpan = np.maximum(np.abs(px) / 120, np.abs(py) / 80)   # >1 outside panel
    hue_border = np.floor(wed * 4) * 0.09 + 0.55 + t * 0.0015
    val_border = 0.45 + 0.30 * np.sin(dpan * 6.0 - t * 0.015)

    hue = np.where(inpanel, hue_panel, hue_border)
    bright = np.where(inpanel, np.where(key, 1.0, 0.50), val_border)
    img = pal(hue) * bright[..., None]

    # thin gold frame around the panel
    frame = np.abs(dpan - 1.0) < 0.02
    img = np.where(frame[..., None], np.array([1.0, 0.95, 0.6], np.float32), img)
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os
    import subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_082.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
