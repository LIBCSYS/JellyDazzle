import numpy as np

W, H = 320, 240
_yy, _xx = np.mgrid[0:H, 0:W].astype(np.float32)


def pal(v):
    a = np.array([0.5, 0.5, 0.5], np.float32)
    b = np.array([0.5, 0.5, 0.5], np.float32)
    c = np.array([1.0, 1.0, 1.0], np.float32)
    d = np.array([0.85, 0.20, 0.55], np.float32)
    return np.clip(a + b * np.cos(2 * np.pi * (c * v[..., None] + d)), 0, 1)


def render(t):
    t = float(t)
    u = np.abs(_xx - 160.0)          # 4-fold mirror coords
    v = np.abs(_yy - 120.0)
    duv = u + v

    # two interleaved chevron systems marching gently toward center
    c1 = 0.5 + 0.5 * np.sin(duv * 0.22 + t * 0.012)
    c2 = 0.5 + 0.5 * np.sin(duv * 0.085 - t * 0.006)
    hue = 0.86 + 0.10 * np.sin(duv * 0.018 - t * 0.004)
    img = pal(hue) * (0.12 + 0.60 * c1 * c2)[..., None]

    # teal accent chevrons (sparse third set)
    c3 = np.clip(np.sin(duv * 0.15 - t * 0.009) - 0.62, 0, 1) / 0.38
    img = img + c3[..., None] * np.array([0.0, 0.45, 0.42], np.float32)

    # corner laser rays fanning toward center
    th = np.arctan2(v + 1e-6, u + 1e-6)
    ray = np.clip(np.sin(th * 12.0 + t * 0.008) - 0.90, 0, 1) / 0.10
    far = np.clip((duv - 130.0) / 100.0, 0, 1)
    img = img + (ray * far)[..., None] * np.array([0.5, 0.35, 0.1], np.float32)

    # central striped core rectangle with slow rolling scanlines
    core = (u < 70) & (v < 26)
    stripes = 0.5 + 0.5 * np.sin(_yy * 0.9 - t * 0.018)
    corecol = pal(np.full_like(u, 0.38) + 0.05 * np.sin(t * 0.005)) \
        * (0.25 + 0.75 * stripes)[..., None]
    img = np.where(core[..., None], corecol, img)
    frame = (u < 74) & (v < 30) & ~core
    img = np.where(frame[..., None], np.array([0.95, 0.88, 0.70], np.float32), img)
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)


if __name__ == "__main__":
    import os
    import subprocess
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz_087.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (img.shape[1], img.shape[0]))
        f.write(img.tobytes())
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview.png")
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", out],
                   check=True, capture_output=True)
