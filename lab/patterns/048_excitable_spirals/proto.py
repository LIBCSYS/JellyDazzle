# 048 excitable_spirals - Greenberg-Hastings excitable CA: glowing wavefronts on dark ground
import numpy as np

W, H = 320, 240
GW, GH = 160, 120
K = 8                      # 0 = rest, 1 = excited, 2..7 refractory
NGEN = 380
SKIP = 100
TAU = 2.0 * np.pi

PA = np.array([0.5, 0.5, 0.5], np.float32)
PB = np.array([0.5, 0.5, 0.5], np.float32)
PC = np.array([1.0, 1.0, 1.0], np.float32)
PD = np.array([0.55, 0.70, 0.85], np.float32)
BASE = np.array([0.04, 0.05, 0.10], np.float32)   # resting background

def pal(v):
    v = np.asarray(v, np.float32)[..., None]
    return PA + PB * np.cos(TAU * (PC * v + PD))

def simulate(seed):
    # sparse symmetric seed: broken wavefront bars -> clean rotating spirals,
    # not the fine-grained turbulence a full-random init produces
    rng = np.random.default_rng(seed)
    q = np.zeros((GH // 2, GW // 2), np.uint8)
    bars = ((14, 20, 58, 1), (34, 8, 46, -1), (50, 28, 70, 1))
    for y, x0, x1, side in bars:
        q[y, x0:x1] = 1                       # excited line
        q[y + side, x0:x1] = K // 2           # refractory backing -> directed wave
    s = np.block([[q, q[:, ::-1]], [q[::-1, :], q[::-1, ::-1]]]).astype(np.uint8)
    hist = []
    for g in range(NGEN):
        exc = (s == 1)
        n = np.zeros(s.shape, np.uint8)
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dx == 0 and dy == 0:
                    continue
                n += np.roll(np.roll(exc, dy, 0), dx, 1)
        fired = (s == 0) & (n >= 1)
        s = np.where(s > 0, (s + 1) % K, 0).astype(np.uint8)
        s[fired] = 1
        hist.append(s.copy())
    return hist

for seed in range(48, 60):
    HIST_L = simulate(seed)
    if (HIST_L[-1] == 1).sum() > 200:      # waves still alive at the end
        break
HIST = np.stack(HIST_L[SKIP:])
NH = len(HIST)

KRON = np.ones((2, 2, 1), np.float32)

def blur3(img):
    out = np.zeros_like(img)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            out += np.roll(np.roll(img, dy, 0), dx, 1)
    return out / 9.0

def colorize(s, drift):
    sf = s.astype(np.float32)
    age = np.clip((sf - 1.0) / (K - 1.0), 0.0, 1.0)
    bright = np.where(s == 0, 0.0, 0.22 + 0.78 * (1.0 - age) ** 1.2)
    col = pal(0.60 + age * 0.25 + drift)
    return BASE + col * bright[..., None]

def render(t):
    gp = (t * 0.12) % (NH - 1)
    i = int(gp)
    f = gp - i
    f = f * f * (3.0 - 2.0 * f)
    drift = t * 0.0005
    lo = colorize(HIST[i], drift) * (1.0 - f) + colorize(HIST[i + 1], drift) * f
    img = blur3(np.kron(lo, KRON))
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
