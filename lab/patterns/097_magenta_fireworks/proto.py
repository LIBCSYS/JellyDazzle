"""097 magenta_fireworks — faithful replica of dazzle.exe R11 (frames d11-d18):
particle fireworks on a hot magenta flood; every burst leaves permanent trails
with gravity droop, the field slowly saturating into multicolor fiber wash.
The ONLY asymmetric routine in the original."""
import numpy as np, os

W, H = 320, 240

_HUES = [(255, 60, 60), (60, 230, 255), (255, 230, 70), (90, 110, 255),
         (110, 255, 110), (255, 150, 40), (240, 240, 240), (255, 90, 200)]

_SPAWN = 9          # frames between bursts
_LIFE = 55          # trail length in frames
_NP = 46            # particles per burst


def render(t):
    img = np.empty((H, W, 3), np.float32)
    img[:] = (185, 8, 118)                                   # hot magenta flood

    te = t + 42                                              # a few bursts already aloft at t=0
    nb = te // _SPAWN + 1
    for k in range(nb):
        t0 = k * _SPAWN
        if t0 > te:
            break
        rg = np.random.default_rng(31 * k + 7)
        cx = rg.uniform(25, W - 25)
        cy = rg.uniform(18, H * 0.66)
        ang = rg.uniform(0, 2 * np.pi, _NP)
        v0 = rg.uniform(0.9, 2.3, _NP)
        # one or two hues per burst, like the original
        c1 = np.array(_HUES[rg.integers(len(_HUES))], np.float32)
        c2 = np.array(_HUES[rg.integers(len(_HUES))], np.float32)
        smax = min(te - t0, _LIFE)
        s = np.arange(smax + 1, dtype=np.float32)
        dec = s * (1.0 - s * 0.004)                          # air drag
        px = cx + np.cos(ang)[:, None] * v0[:, None] * dec[None, :]
        py = cy + np.sin(ang)[:, None] * v0[:, None] * dec[None, :] \
             + 0.018 * s[None, :] ** 2                       # gravity droop
        ix = np.clip(px, 0, W - 1).astype(np.int64).ravel()
        iy = np.clip(py, 0, H - 1).astype(np.int64).ravel()
        # alternate hues across particles; fresh tips glow near-white
        which = (np.arange(_NP) % 2)[:, None].repeat(smax + 1, 1).ravel()
        col = np.where(which[:, None] == 0, c1[None, :], c2[None, :])
        if te - t0 <= _LIFE:                                  # burst still alive
            tip = (s[None, :] > smax - 4).repeat(_NP, 0).ravel()
            col = np.where(tip[:, None], col * 0.35 + 165.0, col)
        img[iy, ix] = col

    # late-run palette darkening toward mud (d17-d18) — gentle, capped
    dim = 1.0 - 0.22 * np.clip((t - 620) / 160.0, 0, 1)
    return np.clip(img * dim, 0, 255).astype(np.uint8)


if __name__ == "__main__":
    img = np.concatenate([render(t) for t in (0, 300, 700)], axis=1)
    ppm = "/tmp/dz097.ppm"
    with open(ppm, "wb") as f:
        f.write(("P6\n%d %d\n255\n" % (img.shape[1], img.shape[0])).encode())
        f.write(img.tobytes())
    here = os.path.dirname(os.path.abspath(__file__))
    os.system("sips -s format png %s --out '%s/preview.png' >/dev/null 2>&1" % (ppm, here))
