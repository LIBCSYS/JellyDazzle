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

rng = np.random.default_rng(75)
JMAX = 26
PH1 = rng.uniform(0, 6.28, JMAX)
PH2 = rng.uniform(0, 6.28, JMAX)
M2 = 5 + (np.arange(JMAX) % 5)
TH = np.linspace(0, 2 * np.pi, 720, endpoint=False)

def render(t):
    T = t + 90
    acc = np.zeros((H, W, 3))
    jf = min(JMAX - 0.001, 3.0 + T * 0.033)
    jv = int(jf)
    cx = W / 2 + 12 * np.sin(t * 0.005)
    cy = H / 2 + 8 * np.cos(t * 0.004)
    for j in range(jv + 1):
        emerge = np.clip(jf - j, 0, 1) ** 0.6
        amp = (0.6 + j * 0.55) * emerge
        drift = t * 0.004 * (1.0 + j * 0.06)
        r = (13 + j * 4.3) * (0.75 + 0.25 * emerge)
        r = r + amp * (0.9 * np.sin(3 * TH + PH1[j] + drift)
                       + 0.8 * np.sin(M2[j] * TH - PH2[j] + drift * 0.7)
                       + 0.4 * np.sin(9 * TH + PH1[j] * 2 - drift * 0.5))
        x = cx + r * np.cos(TH)
        y = cy + r * np.sin(TH) * 0.82
        hue = 0.92 + j * 0.045 + t * 0.0004 + 0.01 * np.sin(TH * 2)
        v = 0.55 + 0.45 * emerge if j == jv else 0.9 - 0.012 * (jv - j)
        cols = hsv(hue, 0.78, max(0.35, v))
        splat(acc, x, y, cols, 1.25)
    acc = soften(acc, 1)
    yy, xx = np.mgrid[0:H, 0:W]
    r2 = ((xx - W / 2) / W) ** 2 + ((yy - H / 2) / H) ** 2
    bg = np.stack([0.012 + 0.01 * (1 - r2), 0.05 + 0.03 * (1 - r2),
                   0.055 + 0.03 * (1 - r2)], -1)
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
