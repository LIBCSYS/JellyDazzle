# 049 voronoi_breath - stained-glass Worley cells with dark walls, seeds orbiting in symmetric rings
import numpy as np

W, H = 320, 240
TAU = 2.0 * np.pi

YY, XX = np.mgrid[0:H, 0:W].astype(np.float32)
X = XX - W / 2.0
Y = YY - H / 2.0

PA = np.array([0.46, 0.46, 0.46], np.float32)
PB = np.array([0.42, 0.42, 0.42], np.float32)
PC = np.array([1.0, 1.0, 1.0], np.float32)
PD = np.array([0.00, 0.333, 0.667], np.float32)

def pal(v):
    v = np.asarray(v, np.float32)[..., None]
    return PA + PB * np.cos(TAU * (PC * v + PD))

RINGS = ((45.0, 6, 0.0016, 0.0040, 0.0),
         (95.0, 6, -0.0011, 0.0030, 0.5),
         (145.0, 6, 0.0008, 0.0050, 1.0))

def render(t):
    pts = [(0.0, 0.0)]
    for rad, n, spd, wob, ph in RINGS:
        rr = rad + 14.0 * np.sin(t * wob + ph)
        for j in range(n):
            a = j * TAU / n + t * spd + ph
            pts.append((rr * np.cos(a), rr * np.sin(a)))
    D = np.stack([np.sqrt((X - px) ** 2 + (Y - py) ** 2) for px, py in pts])
    idx = D.argmin(axis=0)
    P2 = np.partition(D, 1, axis=0)
    F1, F2 = P2[0], P2[1]
    hue = (idx.astype(np.float32) * 0.381966 + t * 0.0004) % 1.0
    img = pal(hue)
    wall = np.clip((F2 - F1) / 10.0, 0.0, 1.0)
    wall = wall * wall * (3.0 - 2.0 * wall)
    img *= (0.22 + 0.78 * wall)[..., None]                 # dark slithering walls
    img *= (0.80 + 0.20 * np.exp(-(F1 * F1) / 800.0))[..., None]  # seed-glow
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
