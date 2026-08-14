# 042 bz_pinwheel - closed-form BZ-style pinwheel: folded spiral wavefronts
import numpy as np

W, H = 320, 240
TAU = 2.0 * np.pi

YY, XX = np.mgrid[0:H, 0:W].astype(np.float32)
X = XX - W / 2.0
Y = YY - H / 2.0
R = np.sqrt(X * X + Y * Y)
TH = np.arctan2(Y, X)
TF = np.abs(np.mod(TH, np.pi / 3.0) - np.pi / 6.0)   # 6-fold mirror fold

PA = np.array([0.5, 0.5, 0.5], np.float32)
PB = np.array([0.5, 0.5, 0.5], np.float32)
PC = np.array([1.0, 1.0, 0.5], np.float32)
PD = np.array([0.80, 0.90, 0.30], np.float32)

def pal(v):
    v = np.asarray(v, np.float32)[..., None]
    return PA + PB * np.cos(TAU * (PC * v + PD))

def render(t):
    p1 = np.cos(TF * 6.0 + R * 0.16 - t * 0.020)
    p2 = np.cos(TF * 10.0 - R * 0.09 + t * 0.013 + 1.7)
    p3 = np.cos(R * 0.05 - t * 0.008)
    v = 0.5 + 0.5 * (0.55 * p1 + 0.30 * p2 + 0.15 * p3)
    img = pal(v * 0.9 + t * 0.0004)
    img *= (1.0 - (R / 220.0) * 0.30)[..., None]          # gentle vignette
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
