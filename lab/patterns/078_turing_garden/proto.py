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

YY, XX = np.mgrid[0:H, 0:W].astype(float)

def blur(a):
    return a * 0.4 + 0.15 * (np.roll(a, 1, 0) + np.roll(a, -1, 0)
                             + np.roll(a, 1, 1) + np.roll(a, -1, 1))

def render(t):
    ph = t * 0.006
    f = (np.sin(XX * 0.085 + ph) + np.sin(YY * 0.079 - ph * 0.8)
         + np.sin((XX * 0.71 + YY * 0.65) * 0.070 + ph * 0.6)
         + np.sin(np.hypot(XX - W / 2, YY - H / 2) * 0.10 - ph * 0.5))
    f = f + 1.1 * np.sin(XX * 0.21 - ph * 0.33) * np.sin(YY * 0.19 + ph * 0.41)
    f = f / 4.5
    k_eff = int(10 + min(18, (t + 60) * 0.026))  # pattern matures with t
    for _ in range(k_eff):
        f = np.tanh(blur(f) * 2.35)
    u = (f + 1) / 2
    # rim light from gradient
    gx = np.roll(u, -1, 1) - np.roll(u, 1, 1)
    gy = np.roll(u, -1, 0) - np.roll(u, 1, 0)
    rim = np.clip(np.abs(gx) + np.abs(gy), 0, 1)
    hueA = 0.52 + 0.08 * np.sin(t * 0.003)
    hueB = hueA + 0.42
    cA = hsv(np.full((H, W), hueA), 0.9, 0.55)
    cB = hsv(np.full((H, W), hueB), 0.85, 0.95)
    img = cA * (1 - u[..., None]) + cB * u[..., None]
    # golden rims crawl along stripe borders
    rimglow = rim ** 1.5 * (0.75 + 0.25 * np.sin(t * 0.02))
    img = img + rimglow[..., None] * np.array([1.0, 0.85, 0.35]) * 0.8
    # gentle center vignette
    r2 = ((XX - W / 2) / W) ** 2 + ((YY - H / 2) / H) ** 2
    img = img * (1.0 - 0.35 * r2)[..., None]
    return (np.clip(img, 0, 1) * 255).astype(np.uint8)

if __name__ == "__main__":
    strip = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    with open("/tmp/x.ppm", "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (strip.shape[1], strip.shape[0]))
        f.write(strip.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run(["sips", "-s", "format", "png", "/tmp/x.ppm",
                    "--out", os.path.join(here, "preview.png")], check=True)
