# 041 cyclic_bloom - cyclic cellular automaton, kaleidoscopic seed, rainbow states
import numpy as np

W, H = 320, 240
GW, GH = 160, 120          # half-res CA grid, drawn as 2x2 blocks
K = 12                     # number of cyclic states
NGEN = 420
SKIP = 120                 # drop the noisy transient

TAU = 2.0 * np.pi
PA = np.array([0.5, 0.5, 0.5], np.float32)
PB = np.array([0.5, 0.5, 0.5], np.float32)
PC = np.array([1.0, 1.0, 1.0], np.float32)
PD = np.array([0.00, 0.333, 0.667], np.float32)

def pal(v):
    v = np.asarray(v, np.float32)[..., None]
    return PA + PB * np.cos(TAU * (PC * v + PD))

# ---- precompute CA history (4-fold mirror symmetric seed, Moore neighborhood) ----
rng = np.random.default_rng(41)
q = rng.integers(0, K, (GH // 2, GW // 2))
s = np.block([[q, q[:, ::-1]], [q[::-1, :], q[::-1, ::-1]]]).astype(np.uint8)

HIST = []
for g in range(NGEN):
    nxt = ((s.astype(np.int16) + 1) % K).astype(np.uint8)
    cnt = np.zeros(s.shape, np.uint8)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            if dx == 0 and dy == 0:
                continue
            cnt += (np.roll(np.roll(s, dy, 0), dx, 1) == nxt)
    s = np.where(cnt >= 1, nxt, s)
    HIST.append(s.copy())
HIST = np.stack(HIST[SKIP:])
NH = len(HIST)

KRON = np.ones((2, 2, 1), np.float32)

def blur3(img):
    out = np.zeros_like(img)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            out += np.roll(np.roll(img, dy, 0), dx, 1)
    return out / 9.0

def render(t):
    gp = (t * 0.15) % (NH - 1)
    i = int(gp)
    f = gp - i
    f = f * f * (3.0 - 2.0 * f)
    drift = t * 0.0006
    c0 = pal(HIST[i].astype(np.float32) / K + drift)
    c1 = pal(HIST[i + 1].astype(np.float32) / K + drift)
    lo = c0 * (1.0 - f) + c1 * f
    img = np.kron(lo, KRON)
    img = blur3(img)
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
