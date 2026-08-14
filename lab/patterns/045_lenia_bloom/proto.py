# 045 lenia_bloom - soft ring-shaped "orbium" cells orbiting in symmetric rings, glowing on dark
import numpy as np

W, H = 320, 240
TAU = 2.0 * np.pi

YY, XX = np.mgrid[0:H, 0:W].astype(np.float32)
X = XX - W / 2.0
Y = YY - H / 2.0
R = np.sqrt(X * X + Y * Y)

PA = np.array([0.45, 0.50, 0.50], np.float32)
PB = np.array([0.50, 0.48, 0.45], np.float32)
PC = np.array([1.0, 1.0, 1.0], np.float32)
PD = np.array([0.55, 0.35, 0.20], np.float32)

def pal(v):
    v = np.asarray(v, np.float32)[..., None]
    return PA + PB * np.cos(TAU * (PC * v + PD))

def ring(d, r0, w):
    return np.exp(-((d - r0) ** 2) / (2.0 * w * w))

def render(t):
    field = np.zeros((H, W), np.float32)
    # central breathing cell
    r0 = 26.0 + 9.0 * np.sin(t * 0.004)
    field += ring(R, r0, 8.0)
    # three rings of orbium cells, overlapping so membranes merge (6-fold symmetric)
    for rad, n, spd, wob, ph, cr, cw in (
            (62.0, 6, 0.0022, 0.0045, 0.0, 16.0, 6.0),
            (88.0, 6, -0.0013, 0.0031, 2.0, 24.0, 9.0),
            (128.0, 12, 0.0010, 0.0038, 0.9, 17.0, 6.5)):
        rr = rad + 18.0 * np.sin(t * wob + ph)
        for j in range(n):
            a = j * TAU / n + t * spd + ph
            px = rr * np.cos(a)
            py = rr * np.sin(a)
            d = np.sqrt((X - px) ** 2 + (Y - py) ** 2)
            field += ring(d, cr, cw) * 0.95
    # faint connective halo so the colony reads as one organism
    field += 0.35 * ring(R, 96.0 + 14.0 * np.sin(t * 0.0027), 34.0)
    L = np.tanh(field * 1.25)
    img = pal(0.62 * L + t * 0.0004) * (0.10 + 0.90 * L)[..., None]
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)

if __name__ == "__main__":
    import os, subprocess
    frames = [render(t) for t in (0, 300, 700)]
    img = np.concatenate(frames, axis=1)
    with open("/tmp/x.ppm", "wb") as fh:
        fh.write(b"P6 %d %d 255\n" % (img.shape[1], img.shape[0]))
        fh.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run(["sips", "-s", "format", "png", "/tmp/x.ppm",
                    "--out", os.path.join(here, "preview.png")],
                   check=True, capture_output=True)
