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

def make_bolt(seed, x_start):
    """Jagged main path + branch definitions; static geometry, growth via t."""
    r = np.random.default_rng(seed)
    NN = 90
    ys = np.linspace(6, H + 30, NN)
    dx = r.normal(0, 5.5, NN)
    dx = np.convolve(dx, np.ones(3) / 3, mode="same")
    xs = x_start + np.cumsum(dx)
    xs -= np.linspace(0, xs[-1] - x_start, NN) * 0.6  # keep roughly vertical
    # arclength per node
    seg = np.hypot(np.diff(xs), np.diff(ys))
    al = np.concatenate([[0], np.cumsum(seg)])
    branches = []
    for bi in range(7):
        k = int(r.uniform(10, NN - 25))
        bn = int(r.uniform(12, 26))
        ang = r.choice([-1, 1]) * r.uniform(0.5, 1.1)
        bdx = np.cos(ang + np.pi / 2) * 4 + r.normal(0, 3.0, bn)
        bdy = np.abs(np.sin(ang + np.pi / 2) * 4 + r.normal(0, 2.0, bn)) + 1.5
        bx = xs[k] + np.cumsum(np.convolve(bdx, np.ones(3) / 3, "same"))
        by = ys[k] + np.cumsum(bdy)
        bseg = np.hypot(np.diff(bx), np.diff(by))
        bal = al[k] + np.concatenate([[0], np.cumsum(bseg)]) * 1.4
        branches.append((bx, by, bal))
    return xs, ys, al, branches

BOLTS = [(make_bolt(741, W * 0.38), 0.72, 0.0),
         (make_bolt(742, W * 0.66), 0.55, 260.0)]

def densify(x, y, a):
    """Interpolate 4x along the polyline."""
    n = len(x)
    u = np.linspace(0, n - 1, n * 4)
    return (np.interp(u, np.arange(n), x), np.interp(u, np.arange(n), y),
            np.interp(u, np.arange(n), a))

def render(t):
    acc = np.zeros((H, W, 3))
    for (xs, ys, al, branches), hue0, toff in BOLTS:
        tip = 70 + (t + toff) * 0.42
        sway = np.sin(t * 0.008 + ys * 0.02) * 3.0
        allpts = [(xs + sway, ys, al)] + \
                 [(bx + np.sin(t * 0.008 + by * 0.02) * 3.0, by, ba)
                  for bx, by, ba in branches]
        for px, py, pa in allpts:
            px, py, pa = densify(px, py, pa)
            vis = pa < tip
            if not vis.any():
                continue
            px, py, pa = px[vis], py[vis], pa[vis]
            fade = np.clip((tip - pa) / 40.0, 0, 1)
            head = np.clip(1.0 - (tip - pa) / 26.0, 0, 1) ** 2
            hue = hue0 + 0.03 * np.sin(pa * 0.02 + t * 0.004)
            core = hsv(hue, 0.25 + 0.3 * (1 - head), 1.0)
            wgt = (0.35 + 0.65 * fade) * (1.0 + 2.2 * head)
            splat(acc, px, py, core, wgt * 0.8)
    glow = soften(acc.copy(), 4)
    acc = soften(acc, 1)
    pulse = 0.85 + 0.15 * np.sin(t * 0.02)
    yy, xx = np.mgrid[0:H, 0:W]
    bg = np.stack([0.015 + 0.02 * (yy / H), 0.005 + 0.005 * (yy / H),
                   0.05 + 0.05 * (yy / H)], -1)
    img = np.clip(bg + (1 - np.exp(-(acc * 0.9 + glow * 1.6))) * pulse, 0, 1)
    return (img * 255).astype(np.uint8)

if __name__ == "__main__":
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    with open("/tmp/x.ppm", "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run(["sips", "-s", "format", "png", "/tmp/x.ppm",
                    "--out", os.path.join(here, "preview.png")], check=True)
