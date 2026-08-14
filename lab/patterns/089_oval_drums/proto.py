import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)


def pal(v):
    a = np.array([0.5, 0.5, 0.5], np.float32)
    b = np.array([0.5, 0.5, 0.5], np.float32)
    c = np.array([1.0, 1.0, 1.0], np.float32)
    d = np.array([0.30, 0.60, 0.90], np.float32)
    return np.clip(a + b * np.cos(2 * np.pi * (c * v[..., None] + d)), 0, 1)


def render(t):
    t = float(t)
    x = _xx - 160.0
    y = _yy - 120.0

    # concentric stadium (racetrack) rings: distance to a horizontal core segment
    aa = 70.0
    dx = np.maximum(np.abs(x) - aa, 0.0)
    d = np.hypot(dx, y)
    sp = 15.0
    ring = np.floor(d / sp - t * 0.008)
    fr = np.mod(d / sp - t * 0.008, 1.0)
    hue = np.mod(ring * 0.13 + t * 0.0012, 1.0)
    img = pal(hue) * (0.35 + 0.60 * np.sin(fr * np.pi))[..., None]

    # perimeter blobs: mirrored soft dots riding the 4th ring
    bx = np.abs(x)
    for (px_, py_) in [(aa + 52.0, 0.0), (0.0, 58.0), (aa * 0.6, 52.0)]:
        db = np.hypot(bx - px_, np.abs(y) - py_)
        blob = np.clip(1.0 - db / 10.0, 0, 1)
        img = img + blob[..., None] * np.array([0.35, 0.1, 0.35], np.float32) \
            * (0.7 + 0.3 * np.sin(t * 0.013 + px_))

    # two striped drums side by side inside the innermost oval, rolling opposite ways
    for sx in (-1.0, 1.0):
        m = (np.abs(x - sx * 38.0) < 31.0) & (np.abs(y) < 21.0)
        g = np.mod(y / 12.0 - t * 0.016 * sx, 1.0)
        dh = np.mod(0.32 - 0.30 * g + t * 0.0012, 1.0)     # green->red drum ramp
        dcol = pal(dh) * (0.35 + 0.65 * (0.5 + 0.5 * np.sin(g * 6.2832)))[..., None]
        img = np.where(m[..., None], dcol, img)
        # drum outline
        mo = (np.abs(np.abs(x - sx * 38.0) - 31.0) < 1.8) & (np.abs(y) < 23.0)
        mo |= (np.abs(np.abs(y) - 21.0) < 1.8) & (np.abs(x - sx * 38.0) < 32.5)
        img = np.where(mo[..., None], np.array([0.95, 0.92, 0.8], np.float32), img)
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os
    import subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_089.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
