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

S = 4200

def layer(acc, t, n_vis, R, kpet, wob, folds, hue0, direction, vscale):
    s = np.arange(n_vis).astype(float)
    phi = s * 0.0075 * direction
    r = R * np.abs(np.sin(phi * kpet + t * 0.0011 * direction)) ** 0.75
    r = r + wob * np.sin(phi * 7 - t * 0.009)
    head = np.clip(1.0 - (n_vis - 1 - s) / 240.0, 0, 1)
    hue = hue0 + s * 0.00008 + t * 0.0004
    cols = hsv(hue, 0.95, (0.5 + 0.5 * np.sin(s * 0.02 + t * 0.01)) * 0.3 + 0.6)
    wgt = (0.5 + 1.6 * head ** 2) * vscale
    rot = t * 0.002 * direction
    for k in range(folds):
        a = rot + k * 2 * np.pi / folds
        x = W / 2 + r * np.cos(phi + a)
        y = H / 2 + r * np.sin(phi + a) * 0.85
        splat(acc, x, y, cols, wgt)
        # mirror copy
        x = W / 2 + r * np.cos(-phi + a)
        y = H / 2 + r * np.sin(-phi + a) * 0.85
        splat(acc, x, y, cols, wgt)

def render(t):
    acc = np.zeros((H, W, 3))
    n_vis = int(min(S, 350 + (t + 60) * 5.1))
    layer(acc, t, n_vis, 105, 2.5, 8.0, 4, 0.78, 1.0, 1.0)
    n2 = int(min(S * 0.7, 240 + (t + 60) * 3.5))
    layer(acc, t, n2, 58, 3.5, 5.0, 4, 0.12, -1.0, 0.8)
    acc = soften(acc, 1)
    yy, xx = np.mgrid[0:H, 0:W]
    r2 = ((xx - W / 2) / W) ** 2 + ((yy - H / 2) / H) ** 2
    bg = np.stack([0.05 + 0.03 * (1 - r2), 0.008 + 0.0 * r2,
                   0.09 + 0.05 * (1 - r2)], -1)
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
