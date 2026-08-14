# 047 target_choir - BZ pacemaker target rings from seven symmetric centers, interfering
import numpy as np

W, H = 320, 240
TAU = 2.0 * np.pi

YY, XX = np.mgrid[0:H, 0:W].astype(np.float32)
X = XX - W / 2.0
Y = YY - H / 2.0

PA = np.array([0.5, 0.5, 0.5], np.float32)
PB = np.array([0.5, 0.5, 0.5], np.float32)
PC = np.array([2.0, 1.0, 0.0], np.float32)
PD = np.array([0.50, 0.20, 0.25], np.float32)

def pal(v):
    v = np.asarray(v, np.float32)[..., None]
    return PA + PB * np.cos(TAU * (PC * v + PD))

# pacemaker centers: origin + hexagonal ring (6-fold symmetric)
CENTERS = [(0.0, 0.0, 0.0)]
for j in range(6):
    a = j * np.pi / 3.0
    CENTERS.append((85.0 * np.cos(a), 85.0 * np.sin(a), 2.0))

DISTS = [np.sqrt((X - px) ** 2 + (Y - py) ** 2) for px, py, _ in CENTERS]

def render(t):
    w = np.zeros((H, W), np.float32)
    for (px, py, ph), d in zip(CENTERS, DISTS):
        w += np.cos(d * 0.21 - t * 0.024 + ph)
    w /= len(CENTERS)
    v = 0.5 + 0.45 * w
    img = pal(v * 0.85 + t * 0.0004)
    img *= (0.80 + 0.20 * w)[..., None]         # wavefront relief
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
