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

rng = np.random.default_rng(77)
NF = 140
SMAX = 260
TH0 = rng.uniform(0, 2 * np.pi, NF)
SPD = rng.uniform(0.6, 1.4, NF)
WAND = np.cumsum(rng.normal(0, 0.085, (NF, SMAX)), axis=1)
WPH = rng.uniform(0, 6.28, NF)
ROSE = rng.random(NF) < 0.3
HUEJ = rng.random(NF)
SS = np.arange(SMAX)

def render(t):
    T = t + 130
    acc = np.zeros((H, W, 3))
    rot = t * 0.0015
    theta = (TH0[:, None] + rot + WAND
             + 0.25 * np.sin(SS[None, :] * 0.03 + WPH[:, None] + t * 0.006))
    x = W / 2 + np.cumsum(np.cos(theta), axis=1) * 1.05
    y = H / 2 + np.cumsum(np.sin(theta), axis=1) * 0.85
    n = np.minimum(SMAX, T * 0.33 * SPD)[:, None]
    grown = SS[None, :] < n
    tip = (SS[None, :] > n - 7) & grown
    hue = np.where(ROSE[:, None], 0.90 + 0.05 * HUEJ[:, None],
                   0.05 + 0.07 * HUEJ[:, None]) + t * 0.0002
    hue = np.broadcast_to(hue, (NF, SMAX))
    val = 0.5 + 0.5 * np.exp(-SS[None, :] / 160.0)
    cols = hsv(hue, 0.85, np.broadcast_to(val, (NF, SMAX)))
    wgt = grown * (0.55 + 0.25 * np.sin(SS[None, :] * 0.15 - t * 0.03)) + tip * 2.2
    splat(acc, x, y, cols.reshape(-1, 3), wgt)
    # glowing spore nodes at fixed arc positions
    node_s = np.arange(40, SMAX, 60)
    for s0 in node_s:
        live = (n[:, 0] > s0)
        if live.any():
            pulse = 1.5 + 1.0 * np.sin(t * 0.025 + s0 * 0.2 + TH0[live])
            splat(acc, x[live, s0], y[live, s0],
                  hsv(hue[live, s0] + 0.04, 0.6, 1.0), pulse)
    acc = soften(acc, 1)
    yy, xx = np.mgrid[0:H, 0:W]
    r2 = ((xx - W / 2) / W) ** 2 + ((yy - H / 2) / H) ** 2
    glow = np.exp(-r2 * 4.0)
    bg = np.stack([0.06 + 0.06 * glow, 0.025 + 0.03 * glow,
                   0.02 + 0.02 * glow], -1)
    img = np.clip(bg + 1 - np.exp(-acc * 0.7), 0, 1)
    return (img * 255).astype(np.uint8)

if __name__ == "__main__":
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    with open("/tmp/x.ppm", "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run(["sips", "-s", "format", "png", "/tmp/x.ppm",
                    "--out", os.path.join(here, "preview.png")], check=True)
