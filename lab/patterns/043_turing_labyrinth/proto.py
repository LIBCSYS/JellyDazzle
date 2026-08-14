# 043 turing_labyrinth - reaction-diffusion labyrinth (iterated band-pass + clip), color flow
import numpy as np

W, H = 320, 240
TAU = 2.0 * np.pi

PA = np.array([0.35, 0.40, 0.45], np.float32)
PB = np.array([0.45, 0.42, 0.40], np.float32)
PC = np.array([1.0, 1.0, 1.0], np.float32)
PD = np.array([0.60, 0.75, 0.90], np.float32)

def pal(v):
    v = np.asarray(v, np.float32)[..., None]
    return PA + PB * np.cos(TAU * (PC * v + PD))

def box3(a):
    b = (np.roll(a, -1, 0) + a + np.roll(a, 1, 0)) / 3.0
    return (np.roll(b, -1, 1) + b + np.roll(b, 1, 1)) / 3.0

# ---- precompute labyrinth field: iterated DoG band-pass with saturation ----
rng = np.random.default_rng(43)
u = rng.standard_normal((H, W)).astype(np.float32)
u = (u + u[::-1, :] + u[:, ::-1] + u[::-1, ::-1]) / 4.0   # 4-fold mirror symmetric

for _ in range(70):
    sb = box3(box3(u))
    lb = sb
    for _ in range(6):
        lb = box3(lb)
    u = np.clip((sb - lb) * 12.0, -1.0, 1.0)

UB = box3(box3(box3(u)))   # softened copy for breathing

def render(t):
    m = 0.5 + 0.5 * np.sin(t * 0.006)          # slow breath 0..1
    uu = u * (1.0 - 0.35 * m) + UB * (0.35 * m)
    v = 0.5 + 0.5 * uu
    img = pal(v * 0.70 + t * 0.0012)
    ridge = (1.0 - np.abs(uu)) ** 2            # bright seams between stripes
    img += ridge[..., None] * 0.22
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
