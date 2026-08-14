# 046 rps_trichrome - three-species rock-paper-scissors domains, spiraling soft-argmax blend
import numpy as np

W, H = 320, 240
TAU = 2.0 * np.pi

YY, XX = np.mgrid[0:H, 0:W].astype(np.float32)
X = XX - W / 2.0
Y = YY - H / 2.0
R = np.sqrt(X * X + Y * Y)
TH = np.arctan2(Y, X)
TF = np.abs(np.mod(TH, np.pi / 3.0) - np.pi / 6.0)   # 6-fold fold

PA = np.array([0.5, 0.5, 0.5], np.float32)
PB = np.array([0.5, 0.5, 0.5], np.float32)
PC = np.array([1.0, 1.0, 1.0], np.float32)
PD = np.array([0.10, 0.40, 0.70], np.float32)

def pal(v):
    v = np.asarray(v, np.float32)
    return PA + PB * np.cos(TAU * (PC * v[..., None] + PD))

def render(t):
    fs = []
    for i in range(3):
        ph = i * TAU / 3.0
        f = (np.cos(TF * 7.0 + R * 0.14 - t * 0.010 + ph)
             + 0.35 * np.cos(R * 0.06 - t * 0.006 + ph * 0.5))
        fs.append(f)
    fs = np.stack(fs)                              # (3,H,W)
    e = np.exp(4.0 * (fs - fs.max(axis=0)))        # soft argmax weights
    wgt = e / e.sum(axis=0)
    drift = t * 0.0005
    img = np.zeros((H, W, 3), np.float32)
    for i in range(3):
        col = pal(np.float32(i / 3.0 + drift))     # (3,) species color
        img += wgt[i][..., None] * col
    top = fs.max(axis=0)
    img *= (0.68 + 0.32 * (0.5 + 0.5 * top))[..., None]   # ridge shading
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
