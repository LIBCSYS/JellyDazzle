# 050 turing_spots - twin hex spot lattices, counter-rotating, spot size breathing
import numpy as np

W, H = 320, 240
TAU = 2.0 * np.pi

YY, XX = np.mgrid[0:H, 0:W].astype(np.float32)
X = XX - W / 2.0
Y = YY - H / 2.0
R = np.sqrt(X * X + Y * Y)

PA = np.array([0.5, 0.5, 0.5], np.float32)
PB = np.array([0.5, 0.5, 0.5], np.float32)
PC = np.array([1.0, 1.0, 1.0], np.float32)
PD = np.array([0.30, 0.55, 0.80], np.float32)

def pal(v):
    v = np.asarray(v, np.float32)[..., None]
    return PA + PB * np.cos(TAU * (PC * v + PD))

def hexlat(k, ang):
    ca, sa = np.cos(ang), np.sin(ang)
    x = X * ca - Y * sa
    y = X * sa + Y * ca
    return (np.cos(x * k)
            + np.cos((x * 0.5 + y * 0.8660254) * k)
            + np.cos((x * 0.5 - y * 0.8660254) * k)) / 3.0

def sstep(x):
    s = np.clip(x * 0.5 + 0.5, 0.0, 1.0)
    return s * s * (3.0 - 2.0 * s)

def render(t):
    f1 = hexlat(0.10, t * 0.0009)
    f2 = hexlat(0.23, -t * 0.0006 + 1.0)
    thr = 0.15 * np.sin(t * 0.005)              # spots swell and shrink
    v1 = sstep((f1 - thr) * 2.5)
    v2 = sstep((f2 + thr) * 2.5)
    mix = 0.55 * v1 + 0.28 * v2 + R * 0.0009 + t * 0.0006
    img = pal(mix)
    img *= (0.70 + 0.30 * v1)[..., None]        # big spots carry the relief
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
