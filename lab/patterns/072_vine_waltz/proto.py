import numpy as np, os, subprocess

W, H = 320, 240

def hsv(h, s, v):
    h = np.asarray(h, dtype=float) % 1.0
    s = np.broadcast_to(np.asarray(s, dtype=float), h.shape)
    v = np.broadcast_to(np.asarray(v, dtype=float), h.shape)
    i = (h * 6).astype(int) % 6
    f = h * 6 - np.floor(h * 6)
    p, q, tt = v * (1 - s), v * (1 - s * f), v * (1 - s * (1 - f))
    r = np.choose(i, [v, q, p, p, tt, v])
    g = np.choose(i, [tt, v, v, q, p, p])
    b = np.choose(i, [p, p, tt, v, v, q])
    return np.stack([r, g, b], -1)

def splat(acc, x, y, cols, w=1.0):
    x = np.asarray(x, dtype=float).ravel()
    y = np.asarray(y, dtype=float).ravel()
    cols = np.asarray(cols, dtype=float).reshape(-1, 3)
    if cols.shape[0] == 1:
        cols = np.broadcast_to(cols, (x.size, 3))
    w = np.broadcast_to(np.asarray(w, dtype=float).ravel(), x.shape)
    xi = np.floor(x).astype(int); yi = np.floor(y).astype(int)
    fx = x - xi; fy = y - yi
    for dx, dy, wt in ((0, 0, (1 - fx) * (1 - fy)), (1, 0, fx * (1 - fy)),
                       (0, 1, (1 - fx) * fy), (1, 1, fx * fy)):
        xx = xi + dx; yy = yi + dy
        m = (xx >= 0) & (xx < W) & (yy >= 0) & (yy < H)
        if m.any():
            np.add.at(acc, (yy[m], xx[m]), cols[m] * (wt[m] * w[m])[:, None])

def soften(a, n=1):
    for _ in range(n):
        a = a * 0.5 + 0.125 * (np.roll(a, 1, 0) + np.roll(a, -1, 0)
                               + np.roll(a, 1, 1) + np.roll(a, -1, 1))
    return a

rng = np.random.default_rng(72)
NV = 14
S = 560
anch = np.linspace(18, W - 18, NV) + rng.uniform(-7, 7, NV)
ph1 = rng.uniform(0, 6.28, NV)
ph2 = rng.uniform(0, 6.28, NV)
spd = rng.uniform(0.75, 1.3, NV)
side = rng.choice([-1.0, 1.0], NV)
hue0 = 0.30 + rng.uniform(-0.04, 0.04, NV)
CURL = int(0.74 * S)

def render(t):
    T = t + 150
    acc = np.zeros((H, W, 3))
    for i in range(NV):
        n = int(min(S, T * spd[i] * 0.95))
        if n < 6:
            continue
        ss = np.arange(n)
        curl = np.where(ss > CURL, (ss - CURL) * 0.006 * side[i], 0.0)
        turn = (0.032 * np.sin(ss * 0.05 + ph1[i])
                + 0.014 * np.sin(ss * 0.013 + ph2[i]) + curl)
        heading = -np.pi / 2 + np.cumsum(turn)
        heading = heading + 0.07 * np.sin(t * 0.014 + ph1[i] + ss * 0.004)
        x = anch[i] + np.cumsum(np.cos(heading)) * 0.72
        y = H - 4 + np.cumsum(np.sin(heading)) * 0.72
        frac = ss / S
        hue = hue0[i] - 0.17 * frac + 0.02 * np.sin(ss * 0.05)
        cols = hsv(hue, 0.85, 0.6 + 0.4 * frac)
        px, py = -np.sin(heading) * 0.6, np.cos(heading) * 0.6
        splat(acc, x, y, cols, 1.2)
        splat(acc, x + px, y + py, cols, 0.8)
        splat(acc, x - px, y - py, cols, 0.8)
        # leaves: small curved fronds alternating sides
        li = np.arange(26, n, 34)
        if li.size:
            sgn = np.where((li // 34) % 2 == 0, 1.0, -1.0) * side[i]
            for k_, r_ in enumerate((1.5, 3.0, 4.5, 6.0, 7.2)):
                la = heading[li] + sgn * (1.35 - k_ * 0.13)
                lx = x[li] + np.cos(la) * r_
                ly = y[li] + np.sin(la) * r_
                lw = 2.2 - k_ * 0.35
                splat(acc, lx, ly,
                      hsv(np.full(li.size, 0.30) + 0.05 * np.sin(li * 0.3), 0.9, 0.85), lw)
        # flower at curled tip
        if n > CURL + 30:
            pulse = 2.5 + 1.2 * np.sin(t * 0.03 + ph2[i])
            fa = np.linspace(0, 6.28, 9)
            for fr in (1.2, 2.6):
                fx = x[-1] + np.cos(fa) * fr
                fy = y[-1] + np.sin(fa) * fr
                splat(acc, fx, fy,
                      hsv(np.full(9, 0.90 + 0.04 * np.sin(ph1[i])), 0.7, 1.0), pulse)
    acc = soften(acc, 1)
    yy = np.mgrid[0:H, 0:W][0] / H
    bg = np.stack([0.10 + 0.05 * (1 - yy), 0.02 + 0.02 * (1 - yy),
                   0.13 + 0.08 * (1 - yy)], -1)
    img = np.clip(bg + 1 - np.exp(-acc * 0.6), 0, 1)
    return (img * 255).astype(np.uint8)

if __name__ == "__main__":
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    with open("/tmp/x.ppm", "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run(["sips", "-s", "format", "png", "/tmp/x.ppm",
                    "--out", os.path.join(here, "preview.png")], check=True)
