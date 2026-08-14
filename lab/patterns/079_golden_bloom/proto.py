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

GA = 2.399963229728653
NMAX = 900

def render(t):
    T = t + 60
    acc = np.zeros((H, W, 3))
    N = int(min(NMAX, 80 + T * 1.2))
    n = np.arange(N).astype(float)
    theta = n * GA + t * 0.0025
    r = 6.4 * np.sqrt(n) * (1.0 + 0.03 * np.sin(t * 0.01))
    x = W / 2 + r * np.cos(theta)
    y = H / 2 + r * np.sin(theta) * 0.80
    hue = n * 0.0045 + t * 0.0008
    # bloom wave rippling outward through the florets
    wave = 0.5 + 0.5 * np.sin(np.sqrt(n) * 1.8 - t * 0.035)
    young = np.clip(1.0 - (N - 1 - n) / 60.0, 0, 1)
    sat = np.clip(0.95 - young * 0.5, 0.5, 0.95)
    val = 0.75 + 0.25 * wave + young * 0.3
    cols = hsv(hue, sat, np.clip(val, 0, 1))
    w = 2.6 + 3.6 * wave + 4.0 * young
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            k = 1.0 if (dx == 0 and dy == 0) else (0.6 if dx * dy == 0 else 0.35)
            splat(acc, x + dx, y + dy, cols, w * k)
    acc = soften(acc, 2)
    yy, xx = np.mgrid[0:H, 0:W]
    r2 = ((xx - W / 2) / W) ** 2 + ((yy - H / 2) / H) ** 2
    bg = np.stack([0.015 + 0.02 * (1 - r2), 0.04 + 0.03 * (1 - r2),
                   0.015 + 0.02 * (1 - r2)], -1)
    img = np.clip(bg + 1 - np.exp(-acc * 0.85), 0, 1)
    return (img * 255).astype(np.uint8)

if __name__ == "__main__":
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    with open("/tmp/x.ppm", "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run(["sips", "-s", "format", "png", "/tmp/x.ppm",
                    "--out", os.path.join(here, "preview.png")], check=True)
