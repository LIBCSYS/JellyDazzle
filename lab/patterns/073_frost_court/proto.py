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

def arm_points(t):
    """One arm of the snowflake along +x, returns x, y, s (distance from center)."""
    T = t + 250
    L = min(105.0, T * 0.16)
    xs, ys, ds = [], [], []
    spine = np.arange(0, L, 1.0)
    xs.append(spine); ys.append(np.zeros_like(spine)); ds.append(spine)
    ca, sa = np.cos(np.pi / 3), np.sin(np.pi / 3)
    for node in np.arange(8.0, L, 8.0):
        bmax = (108.0 - node) * 0.42
        lb = min(bmax, max(0.0, (L - node) * 0.85))
        if lb < 2:
            continue
        u = np.arange(0, lb, 1.0)
        for sgn in (1.0, -1.0):
            xs.append(node + u * ca)
            ys.append(sgn * u * sa)
            ds.append(node + u)
        # tiny secondary spurs
        for node2 in np.arange(6.0, lb, 10.0):
            l2 = min(6.0, (lb - node2) * 0.7)
            if l2 < 1.5:
                continue
            u2 = np.arange(0, l2, 1.0)
            for sgn in (1.0, -1.0):
                xs.append(node + node2 * ca + u2)
                ys.append(sgn * (node2 * sa + u2 * 0.15))
                ds.append(node + node2 + u2)
    return np.concatenate(xs), np.concatenate(ys), np.concatenate(ds)

def render(t):
    acc = np.zeros((H, W, 3))
    x0, y0, s0 = arm_points(t)
    rot0 = t * 0.0012
    shim = 0.62 + 0.38 * np.sin(s0 * 0.22 - t * 0.045)
    hue = 0.52 + 0.06 * np.sin(s0 * 0.05 + t * 0.002)
    sat = np.clip(0.75 - 0.65 * np.exp(-s0 / 18.0), 0.08, 0.8)
    cols = hsv(hue, sat, shim)
    for k in range(6):
        a = rot0 + k * np.pi / 3
        ca, sa = np.cos(a), np.sin(a)
        X = W / 2 + x0 * ca - y0 * sa
        Y = H / 2 + x0 * sa + y0 * ca
        splat(acc, X, Y, cols, 1.0)
    # center hexagonal plate
    T = t + 250
    hr = min(13.0, T * 0.05)
    th = np.linspace(0, 2 * np.pi, 120, endpoint=False)
    hexr = hr / np.cos((th + rot0) % (np.pi / 3) - np.pi / 6)
    splat(acc, W / 2 + hexr * np.cos(th), H / 2 + hexr * np.sin(th),
          hsv(np.full(120, 0.53), 0.35, 0.9), 1.2)
    acc = soften(acc, 1)
    yy, xx = np.mgrid[0:H, 0:W]
    r2 = ((xx - W / 2) / W) ** 2 + ((yy - H / 2) / H) ** 2
    glow = np.exp(-r2 * 5.0)
    bg = np.stack([0.01 + 0.02 * glow, 0.02 + 0.05 * glow,
                   0.07 + 0.13 * glow], -1)
    img = np.clip(bg + 1 - np.exp(-acc * 0.8), 0, 1)
    return (img * 255).astype(np.uint8)

if __name__ == "__main__":
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    with open("/tmp/x.ppm", "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run(["sips", "-s", "format", "png", "/tmp/x.ppm",
                    "--out", os.path.join(here, "preview.png")], check=True)
