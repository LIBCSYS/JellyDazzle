# 044 majority_quilt - majority-vote CA coarsening, scrubbed as slow blob morphing
import numpy as np

W, H = 320, 240
GW, GH = 160, 120
NGEN = 150
TAU = 2.0 * np.pi

PA = np.array([0.5, 0.5, 0.5], np.float32)
PB = np.array([0.5, 0.5, 0.5], np.float32)
PC = np.array([1.0, 0.7, 0.4], np.float32)
PD = np.array([0.00, 0.15, 0.20], np.float32)

def pal(v):
    v = np.asarray(v, np.float32)[..., None]
    return PA + PB * np.cos(TAU * (PC * v + PD))

def box3(a):
    b = (np.roll(a, -1, 0) + a + np.roll(a, 1, 0)) / 3.0
    return (np.roll(b, -1, 1) + b + np.roll(b, 1, 1)) / 3.0

# ---- precompute majority-vote history (4-fold mirror symmetric +-1 seed) ----
rng = np.random.default_rng(44)
q = rng.choice(np.array([-1, 1], np.int8), (GH // 2, GW // 2))
u = np.block([[q, q[:, ::-1]], [q[::-1, :], q[::-1, ::-1]]]).astype(np.int8)

SHIST = []
for g in range(NGEN):
    n = np.zeros(u.shape, np.int16)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            n += np.roll(np.roll(u, dy, 0), dx, 1)
    u = np.where(n > 0, 1, -1).astype(np.int8)
    SHIST.append(box3(box3(u.astype(np.float32))))
SHIST = np.stack(SHIST)

KRON = np.ones((2, 2, 1), np.float32)

def render(t):
    lo_g = 22                                   # skip the noisiest early gens
    span = NGEN - 1 - lo_g
    m = (t * 0.09) % (2 * span)
    gp = lo_g + (m if m <= span else 2 * span - m)   # ping-pong scrub
    i = int(gp)
    f = gp - i
    f = f * f * (3.0 - 2.0 * f)
    j = min(i + 1, NGEN - 1)
    val = SHIST[i] * (1.0 - f) + SHIST[j] * f   # smooth field in [-1,1]
    drift = t * 0.0005
    img = pal(0.5 + 0.32 * val + drift)
    img *= (0.65 + 0.35 * np.abs(val))[..., None]  # darken seams between patches
    img = np.kron(img, KRON)
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
