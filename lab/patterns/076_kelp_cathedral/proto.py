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

rng = np.random.default_rng(76)
NS = 17
anch = np.linspace(12, W - 12, NS) + rng.uniform(-6, 6, NS)
ph = rng.uniform(0, 6.28, NS)
spd = rng.uniform(0.75, 1.3, NS)
amp = rng.uniform(9, 17, NS)
hue_s = 0.26 + rng.uniform(0, 0.12, NS)
hue_s[rng.random(NS) < 0.2] = 0.94  # a few red-algae strands
far = rng.random(NS) < 0.35          # dim background layer

def render(t):
    T = t + 160
    acc = np.zeros((H, W, 3))
    yy, xx = np.mgrid[0:H, 0:W]
    # water: vertical gradient + slow god rays
    depth = yy / H
    ray = (0.5 + 0.5 * np.sin(xx * 0.045 - t * 0.006 + np.sin(yy * 0.01))) ** 4
    ray = ray * (1 - depth) ** 1.6 * 0.35
    bg = np.stack([0.01 + 0.015 * (1 - depth) + ray * 0.15,
                   0.05 + 0.10 * (1 - depth) + ray * 0.5,
                   0.10 + 0.13 * (1 - depth) + ray * 0.55], -1)
    for i in range(NS):
        depth_mul = 0.35 if far[i] else 1.0
        Li = min(H - (16.0 if not far[i] else 60.0), T * 0.35 * spd[i])
        n = int(Li)
        if n < 6:
            continue
        ys = H - np.arange(n).astype(float)
        rise = (H - ys) / H
        sway = np.sin(ys * 0.028 + t * 0.018 * spd[i] + ph[i]) * amp[i] * rise ** 1.2
        sway = sway + np.sin(ys * 0.011 - t * 0.011 + ph[i] * 2) * 4.0 * rise
        x = anch[i] + sway
        v = (0.5 + 0.5 * rise) * depth_mul
        cols = hsv(hue_s[i] + 0.05 * np.sin(ys * 0.02 + ph[i]), 0.85, v)
        for xoff, wo in ((-0.9, 0.8), (0.0, 1.5), (0.9, 0.8)):
            splat(acc, x + xoff, ys, cols, wo * depth_mul)
        # blades: curved fronds alternating sides, waving
        bi = np.arange(18, n, 24)
        if bi.size:
            sgn = np.where(bi // 24 % 2 == 0, 1.0, -1.0)
            flex = 0.6 + 0.4 * np.sin(t * 0.02 + bi * 0.5 + ph[i])
            for k_, r_ in enumerate((1.5, 3.0, 4.5, 6.0, 7.5, 9.0, 10.5)):
                bx = x[bi] + sgn * r_
                by = ys[bi] - r_ * 0.55 * flex + (r_ ** 2) * 0.045
                bw = max(0.25, 2.4 - k_ * 0.3) * depth_mul
                bh = 0.20 + 0.10 * np.sin(bi * 0.3) + 0.04 * k_
                splat(acc, bx, by, hsv(bh, 0.9, 0.9 * depth_mul), bw)
        # gold shimmer tip
        if not far[i]:
            glim = 2.0 + 1.2 * np.sin(t * 0.025 + ph[i])
            splat(acc, x[-3:], ys[-3:], np.array([[1.0, 0.85, 0.3]]), glim)
    # bubbles
    for k in range(20):
        byk = H - ((t * 0.7 + k * 53.0) % (H + 30.0))
        bxk = (anch[k % NS] + 10 * np.sin(t * 0.02 + k)) % W
        splat(acc, np.array([bxk]), np.array([byk]), np.array([[0.5, 0.8, 0.9]]), 1.2)
    acc = soften(acc, 1)
    img = np.clip(bg + 1 - np.exp(-acc * 0.75), 0, 1)
    return (img * 255).astype(np.uint8)

if __name__ == "__main__":
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    with open("/tmp/x.ppm", "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run(["sips", "-s", "format", "png", "/tmp/x.ppm",
                    "--out", os.path.join(here, "preview.png")], check=True)
