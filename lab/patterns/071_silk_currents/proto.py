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

rng = np.random.default_rng(71)
NP_ = 800
p_hue = rng.random(NP_)
p_x0 = rng.random(NP_) * W
p_y0 = rng.random(NP_) * H

def field_angle(x, y, ph):
    return 1.15 * (np.sin(x * 0.021 + ph) + np.sin(y * 0.017 - ph * 0.7)
                   + np.sin((x + y) * 0.011 + ph * 0.4)
                   + np.sin(np.hypot(x - W / 2, y - H / 2) * 0.02 - ph * 0.5))

def render(t):
    T = t * 0.02
    acc = np.zeros((H, W, 3))
    x = (p_x0 + np.sin(T * 0.31 + p_hue * 6.28) * 14) % W
    y = (p_y0 + np.cos(T * 0.23 + p_hue * 6.28) * 10) % H
    hue = (0.52 + p_hue * 0.45 + t * 0.0006)
    steps = 110
    for s in range(steps):
        a = field_angle(x, y, T * 0.35)
        x = x + np.cos(a) * 1.6
        y = y + np.sin(a) * 1.6
        w = 0.05 + 0.55 * (s / steps) ** 2
        cols = hsv(hue + s * 0.0012, 0.85, 1.0)
        splat(acc, x, y, cols, w)
    acc = soften(acc, 1)
    yy, xx = np.mgrid[0:H, 0:W]
    r2 = ((xx - W / 2) / W) ** 2 + ((yy - H / 2) / H) ** 2
    bg = np.stack([0.03 + 0.02 * (1 - r2), 0.01 + 0.015 * (1 - r2),
                   0.10 + 0.06 * (1 - r2)], -1)
    img = np.clip(bg + 1 - np.exp(-acc * 0.55), 0, 1)
    return (img * 255).astype(np.uint8)

if __name__ == "__main__":
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    with open("/tmp/x.ppm", "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run(["sips", "-s", "format", "png", "/tmp/x.ppm",
                    "--out", os.path.join(here, "preview.png")], check=True)
